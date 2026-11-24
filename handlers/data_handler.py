import numpy as np

class DataHandler:
    def __init__(self, max_samples: int = 20):
        self.max_samples = max_samples
        self.actual_sample = max_samples   
        self.has_data = True

        # 12 parameters:
        # ax, ay, az, pitch, roll, pitch_f, roll_f,
        # freeFall, impact, tilt, postFall, detected
        self.env_values = np.zeros((12, max_samples))

    def shift_left(self):
        self.env_values[:, :-1] = self.env_values[:, 1:]

    def update_data(self, line: str) -> None:
        parts = line.strip().split(",")

        if len(parts) != 12:
            return

        new_values = np.array([float(v) for v in parts])

        if self.actual_sample < self.max_samples:
            self.env_values[:, self.actual_sample] = new_values

            if self.actual_sample > 1:
                self.has_data = True

            self.actual_sample += 1

        else:
            self.shift_left()
            self.env_values[:, -1] = new_values
            self.has_data = True