import unittest
from serial import SerialException
from unittest.mock import MagicMock, patch
from handlers.serial_handler import SerialHandler


class TestSerialHandler(unittest.TestCase):
    
    @patch("serial.Serial")
    def test_successful_connection(self, mock_serial):
        mock_serial.return_value.in_waiting = 0
        handler = SerialHandler(port="COM_TEST")

        self.assertTrue(handler.connected)
        self.assertIsNotNone(handler.port)

    @patch("serial.Serial", side_effect=SerialException("Port error"))
    def test_failed_connection(self, mock_serial):
        handler = SerialHandler(port="INVALID")

        self.assertFalse(handler.connected)
        self.assertIsNone(handler.port)

    @patch("serial.Serial")
    def test_read_line_with_data(self, mock_serial):
        mock_port = MagicMock()
        mock_port.in_waiting = 1
        mock_port.readline.return_value = b"1,2,3\n"
        mock_serial.return_value = mock_port

        handler = SerialHandler(port="COM_TEST")
        line = handler.read_line()

        self.assertEqual(line, "1,2,3")

    @patch("serial.Serial")
    def test_read_line_no_data(self, mock_serial):
        mock_port = MagicMock()
        mock_port.in_waiting = 0
        mock_serial.return_value = mock_port

        handler = SerialHandler(port="COM_TEST")
        line = handler.read_line()

        self.assertEqual(line, "")


if __name__ == "__main__":
    unittest.main()
