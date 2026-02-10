import unittest
from unittest.mock import MagicMock, patch

from handlers.hmi import GraphicalInterface
from handlers.data_handler import DataHandler
from handlers.serial_handler import SerialHandler


class TestGraphicalInterface(unittest.TestCase):

    @patch("handlers.hmi.FigureCanvasTkAgg")
    @patch("handlers.hmi.animation.FuncAnimation")
    @patch("handlers.hmi.tk.Tk")
    def setUp(self, mock_tk, _, mock_canvas):
        # Mock Tk root
        mock_root = MagicMock()
        mock_tk.return_value = mock_root

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

    def test_serial_thread_handles_empty_read(self):
        self.serial_handler.read_line.return_value = ""

        with patch("time.sleep", side_effect=Exception("stop")):
            try:
                self.app.serial_reader_thread()
            except Exception:
                pass  


if __name__ == "__main__":
    unittest.main()
