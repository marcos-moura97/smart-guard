
import serial

class SerialHandler:
    def __init__(self, port: str = "COM1", baudrate: int = 9600, timeout: int = 1):
        self.connected = False
        self.port = None
        try:
            self.port = serial.Serial(port, baudrate, timeout=timeout)
            self.connected = True
            print("Serial port connected.")
        except serial.SerialException:
            print(f"{port} not found. Running without serial input.")

    def read_line(self) -> str:
        if self.connected and self.port.in_waiting:
            return self.port.readline().decode("utf-8").strip()
        return ""