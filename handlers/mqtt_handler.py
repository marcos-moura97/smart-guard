import json
import queue
import re
import time

import paho.mqtt.client as mqtt

CRITICAL_FLAGS = ("free_fall", "fall_detected")
EVENT_FLAGS = ("free_fall", "impact", "tilt_change", "post_fall_monitor", "fall_detected")

_MISSING_COMMA = re.compile(r'("delta_time"\s*:\s*-?\d+)\s*(?=")')


def parse_packet(payload: bytes) -> dict:
    text = payload.decode("utf-8")
    try:
        return json.loads(text)
    except json.JSONDecodeError:
        return json.loads(_MISSING_COMMA.sub(r"\1,", text))


def packet_to_line(data: dict) -> str:
    ax, ay, az = data["imu"]["accel"]
    ev = data["events"]
    values = [
        f"{ax:.3f}", f"{ay:.3f}", f"{az:.3f}",
        f"{data['dir']['pitch']:.3f}", f"{data['dir']['roll']:.3f}",
        f"{data['dir_filtered']['pitch']:.3f}", f"{data['dir_filtered']['roll']:.3f}",
        *(str(int(ev[k])) for k in EVENT_FLAGS),
    ]
    return ",".join(values)


class MQTTHandler:
    def __init__(
        self,
        broker: str = "localhost",
        port: int = 1883,
        topic: str = "/data",
        event_topic: str = "/events",
        data_qos: int = 0,
        event_qos: int = 2,
        client_id: str = "smartguard-hmi",
        client=None,
    ):
        self.topic = topic
        self.event_topic = event_topic
        self.data_qos = data_qos
        self.event_qos = event_qos
        self.connected = False

        self.queue = queue.Queue(maxsize=1000)
        self.event_queue = queue.Queue()
        self.stats = {"data": 0, "events": 0, "invalid": 0}

        self.client = client or mqtt.Client(
            mqtt.CallbackAPIVersion.VERSION2, client_id=client_id, clean_session=False
        )
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect
        self.client.on_message = self.on_message
        self.client.reconnect_delay_set(min_delay=1, max_delay=10)
        try:
            self.client.connect_async(broker, port, keepalive=60)
            self.client.loop_start()
        except (ConnectionRefusedError, OSError) as e:
            print(f"MQTT error: {e}")

    def subscriptions(self) -> list:
        return [(self.topic, self.data_qos), (self.event_topic, self.event_qos)]

    def on_connect(self, client, userdata, flags, reason_code, properties=None):
        if reason_code == 0:
            self.connected = True
            print("MQTT connected.")

            client.subscribe(self.subscriptions())
        else:
            print(f"MQTT connection failed: {reason_code}")

    def on_disconnect(self, client, userdata, flags, reason_code, properties=None):
        self.connected = False

    def on_message(self, client, userdata, msg):
        try:
            data = parse_packet(msg.payload)
            if msg.topic == self.event_topic:
                flags = {k: int(data["events"][k]) for k in EVENT_FLAGS if k in data["events"]}
                self.event_queue.put((time.time(), flags))
                self.stats["events"] += 1
            else:
                line = packet_to_line(data)
                try:
                    self.queue.put_nowait(line)
                except queue.Full:
                    self.queue.get_nowait()   # keep the newest samples
                    self.queue.put_nowait(line)
                self.stats["data"] += 1
        except (UnicodeDecodeError, ValueError, KeyError, IndexError, TypeError):
            self.stats["invalid"] += 1

    def read_line(self) -> str:
        try:
            return self.queue.get_nowait()
        except queue.Empty:
            return ""

    def read_event(self):
        try:
            return self.event_queue.get_nowait()
        except queue.Empty:
            return None

    def close(self):
        self.client.loop_stop()
        self.client.disconnect()