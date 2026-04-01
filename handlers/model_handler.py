import tensorflow as tf
import numpy as np
from typing import Tuple
from .data_handler import DataHandler

class FallModelHandler:
    def __init__(
        self,
        model_path: str,
        window_size: int = 200,
        threshold: float = 0.7
    ):
        self.window_size = window_size
        self.threshold = threshold

        try:
            self.model = tf.keras.models.load_model(model_path)
            print("Fall Detection Model loaded successfully.")
        except Exception as e:
            print(f"Error loading model: {e}")
            self.model = None

    def is_ready(self, data_handler) -> bool:
        return data_handler.actual_sample >= self.window_size

    def preprocess(self, data_handler) -> np.ndarray:

        # Extract last N samples
        data = data_handler.env_values[:, -self.window_size:]
        data = data.T
        # Example using IMU only (ax, ay, az, pitch, roll, pitch_f, roll_f)
        features = data[:, :7]

        return np.expand_dims(features, axis=0)

    def predict(self, data_handler: DataHandler) -> Tuple[bool, float]:
        if self.model is None:
            return False, 0.0

        if not self.is_ready(data_handler):
            return False, 0.0

        try:
            model_input = self.preprocess(data_handler)
            proba = float(self.model.predict(model_input, verbose=0)[0])

            detected = proba > self.threshold
            return detected, proba

        except Exception as e:
            print(f"Inference error: {e}")
            return False, 0.0