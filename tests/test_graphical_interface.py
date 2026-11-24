import csv
import os
import tempfile
import unittest
import time
from unittest.mock import MagicMock, patch

import matplotlib
matplotlib.use("Agg")

from handlers.hmi import GraphicalInterface
from handlers.data_handler import DataHandler
from handlers.serial_handler import SerialHandler


class TestGraphicalInterface(unittest.TestCase):

    def setUp(self):
        # replace every Tk widget with a mock: the tests need no display
        patchers = [patch(f"handlers.hmi.tk.{w}") for w in ("Tk", "Frame", "Label", "Listbox", "Button")]
        patchers += [patch("handlers.hmi.animation.FuncAnimation")]
        mocks = [p.start() for p in patchers]
        for p in patchers:
            self.addCleanup(p.stop)
        mock_canvas = patch("handlers.hmi.FigureCanvasTkAgg").start()
        self.addCleanup(patch.stopall)
        # every Label() call returns its own mock, so the event labels can be told apart
        mocks[2].side_effect = lambda *a, **k: MagicMock()
        mocks[3].return_value.size.return_value = 0   # Listbox starts empty

        mock_canvas_instance = MagicMock()
        mock_canvas.return_value = mock_canvas_instance

        self.serial_handler = MagicMock(spec=SerialHandler)
        self.data_handler = DataHandler(max_samples=10)

        self.app = GraphicalInterface(self.serial_handler, self.data_handler)

    def test_app_initialization(self):
        self.assertIsNotNone(self.app.root)
        self.assertEqual(len(self.app.event_labels), 5)
        self.assertEqual(len(self.app.lines), 5)

    def test_update_event_label_true(self):
        label = MagicMock()
        self.app.update_event_label(label, 1)
        label.config.assert_called_with(text="True", fg="green")

    def test_update_event_label_false(self):
        label = MagicMock()
        self.app.update_event_label(label, 0)
        label.config.assert_called_with(text="False", fg="red")

    def test_update_without_data(self):
        self.data_handler.has_data = False
        result = self.app.update(None)
        self.assertEqual(result, self.app.lines)

    def test_update_with_valid_data(self):
        line = "1,2,3,10,20,11,21,0,1,0,1,1"
        self.app.data_queue.put(line)

        result = self.app.update(None)

        self.assertEqual(result, self.app.lines)
        self.assertTrue(self.data_handler.has_data)

    def test_serial_handler_has_no_event_topic(self):
        # SerialHandler has no read_event: polling must be a no-op
        self.app.poll_critical_events()
        self.assertEqual(self.app.critical_state["free_fall"][0], 0)

    def _app_with_events(self, events):
        handler = MagicMock()
        handler.read_event.side_effect = list(events) + [None]
        handler.stats = {"data": 0, "events": len(events), "invalid": 0}
        self.app.connection_handler = handler
        self.app.event_log = MagicMock()
        self.app.event_log.size.return_value = 1

    def test_critical_event_is_logged_and_held(self):
        t0 = time.time()
        self._app_with_events([(t0, {"free_fall": 1}), (t0 + 0.1, {"free_fall": 0})])
        self.app.poll_critical_events()
        # rising and falling edge both logged
        self.assertEqual(self.app.event_log.insert.call_count, 2)
        self.assertEqual(self.app.critical_state["free_fall"], (0, t0))
        # the 0.1 s pulse is still shown as True during the hold time, even if the stream missed it
        self.assertEqual(self.app.critical_value("free_fall", 0, t0 + 0.5), 1)
        self.assertEqual(self.app.critical_value("free_fall", 0, t0 + 2.0), 0)

    def test_repeated_event_state_not_logged_twice(self):
        t0 = time.time()
        self._app_with_events([(t0, {"fall_detected": 1}), (t0 + 0.1, {"fall_detected": 1})])
        self.app.poll_critical_events()
        self.assertEqual(self.app.event_log.insert.call_count, 1)
        self.assertEqual(self.app.critical_value("fall_detected", 0, t0 + 60), 1)

    def test_update_shows_reliable_flag_when_stream_lost_it(self):
        # stream sample without the flag (the packet that carried it was lost on QoS 0)
        self.app.data_queue.put("0,0,1,0,0,0,0,0,0,0,0,0")
        self._app_with_events([(time.time(), {"free_fall": 1})])
        self.app.update(None)
        self.app.event_labels[0].config.assert_called_with(text="True", fg="green")
        self.app.event_labels[4].config.assert_called_with(text="False", fg="red")

    def test_stream_flags_are_logged_on_change(self):
        self.app.event_log = MagicMock()
        self.app.event_log.size.return_value = 1
        for line in ["0,0,1,0,0,0,0,0,1,0,0,0",    # impact rises
                     "0,0,1,0,0,0,0,0,1,0,0,0",    # no change: not logged again
                     "0,0,1,0,0,0,0,0,0,1,1,0"]:   # impact falls, tilt + post-fall rise
            self.app.data_queue.put(line)
        self.app.update(None)
        logged = [(f, v) for _, f, v in self.app.log_entries]
        self.assertEqual(logged, [("impact", 1), ("impact", 0), ("tilt_change", 1),
                                  ("post_fall_monitor", 1)])

    def test_stream_flags_ignore_malformed_line(self):
        self.app.event_log = MagicMock()
        self.app.track_stream_flags("garbage", time.time())
        self.assertEqual(self.app.log_entries, [])

    def test_log_title_and_export_button(self):
        self.assertTrue(hasattr(self.app, "export_button"))
        self.assertFalse(hasattr(self.app, "link_label"))

    def test_write_log_keeps_every_entry(self):
        t0 = time.time()
        events = [(t0 + i * 0.1, {"free_fall": (i + 1) % 2}) for i in range(20)]
        self._app_with_events(events)
        self.app.poll_critical_events()
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "log.csv")
            self.assertEqual(self.app.write_log(path), 20)   # more than the 12 rows shown
            with open(path, newline="") as f:
                rows = list(csv.DictReader(f))
        self.assertEqual(len(rows), 20)
        self.assertEqual((rows[0]["flag"], rows[0]["value"]), ("free_fall", "1"))
        self.assertEqual(rows[1]["value"], "0")

    def test_export_log_cancelled_writes_nothing(self):
        with patch("handlers.hmi.filedialog.asksaveasfilename", return_value=""), \
             patch.object(self.app, "write_log") as write:
            self.app.export_log()
        write.assert_not_called()

    def test_export_log_saves_to_chosen_file(self):
        with patch("handlers.hmi.filedialog.asksaveasfilename", return_value="/x/log.csv"), \
             patch("handlers.hmi.messagebox.showinfo") as info, \
             patch.object(self.app, "write_log", return_value=3) as write:
            self.app.export_log()
        write.assert_called_once_with("/x/log.csv")
        info.assert_called_once()

    def test_serial_thread_handles_empty_read(self):
        self.serial_handler.read_line.return_value = ""

        with patch("time.sleep", side_effect=Exception("stop")):
            try:
                self.app.serial_reader_thread()
            except Exception:
                pass  


if __name__ == "__main__":
    unittest.main()