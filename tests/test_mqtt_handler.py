import json
import unittest
from types import SimpleNamespace
from unittest.mock import MagicMock

from handlers.mqtt_handler import MQTTHandler, packet_to_line, parse_packet


def packet(free_fall=0, fall_detected=0):
    return {
        "delta_time": 20000,
        "imu": {"accel": [0.1, 0.2, 0.97], "gyro": [1.0, 2.0, 3.0]},
        "dir": {"pitch": 1.5, "roll": 2.5},
        "dir_filtered": {"pitch": 1.25, "roll": 2.25},
        "events": {"free_fall": free_fall, "impact": 1, "tilt_change": 0,
                   "post_fall_monitor": 1, "fall_detected": fall_detected},
    }


def msg(topic, payload):
    return SimpleNamespace(topic=topic, payload=payload if isinstance(payload, bytes) else
                           json.dumps(payload).encode())


class TestMQTTHandler(unittest.TestCase):

    def setUp(self):
        self.client = MagicMock()
        self.h = MQTTHandler(topic="/data", event_topic="/events", client=self.client)

    def test_connects_without_blocking(self):
        self.client.connect_async.assert_called_once()
        self.client.loop_start.assert_called_once()

    def test_subscribes_with_qos_per_topic(self):
        self.h.on_connect(self.client, None, None, 0, None)
        self.client.subscribe.assert_called_once_with([("/data", 0), ("/events", 2)])
        self.assertTrue(self.h.connected)

    def test_failed_connect_does_not_subscribe(self):
        self.h.on_connect(self.client, None, None, 5, None)
        self.client.subscribe.assert_not_called()
        self.assertFalse(self.h.connected)

    def test_data_packet_goes_to_stream_queue_in_order(self):
        self.h.on_message(self.client, None, msg("/data", packet()))
        self.h.on_message(self.client, None, msg("/data", packet(free_fall=1)))
        self.assertEqual(self.h.read_line(), "0.100,0.200,0.970,1.500,2.500,1.250,2.250,0,1,0,1,0")
        self.assertEqual(self.h.read_line(), "0.100,0.200,0.970,1.500,2.500,1.250,2.250,1,1,0,1,0")
        self.assertEqual(self.h.read_line(), "")   # no duplicates once consumed
        self.assertIsNone(self.h.read_event())

    def test_event_packet_goes_to_event_queue(self):
        self.h.on_message(self.client, None, msg("/events", packet(free_fall=1)))
        t, flags = self.h.read_event()
        self.assertEqual(flags["free_fall"], 1)
        self.assertEqual(flags["fall_detected"], 0)
        self.assertEqual(self.h.read_line(), "")   # events do not feed the plots
        self.assertEqual(self.h.stats["events"], 1)

    def test_invalid_payload_is_counted_not_raised(self):
        self.h.on_message(self.client, None, msg("/data", b"not json"))
        self.h.on_message(self.client, None, msg("/data", {"imu": {}}))
        self.assertEqual(self.h.stats["invalid"], 2)
        self.assertEqual(self.h.read_line(), "")

    def test_firmware_json_with_missing_comma_is_repaired(self):
        raw = json.dumps(packet()).replace('"delta_time": 20000, ', '"delta_time": 20000')
        with self.assertRaises(json.JSONDecodeError):
            json.loads(raw)
        self.assertEqual(parse_packet(raw.encode())["delta_time"], 20000)

    def test_stream_queue_keeps_newest_when_full(self):
        for i in range(self.h.queue.maxsize + 5):
            self.h.on_message(self.client, None, msg("/data", packet()))
        self.assertEqual(self.h.queue.qsize(), self.h.queue.maxsize)
        self.assertEqual(self.h.stats["data"], self.h.queue.maxsize + 5)

    def test_packet_to_line_has_12_values(self):
        self.assertEqual(len(packet_to_line(packet()).split(",")), 12)


if __name__ == "__main__":
    unittest.main()