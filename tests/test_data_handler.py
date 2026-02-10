import unittest
import numpy as np

from handlers.data_handler import DataHandler

class TestDataHandler(unittest.TestCase):

    def setUp(self):
        self.handler = DataHandler(max_samples=5)

    def test_initial_state(self):
        self.assertEqual(self.handler.env_values.shape, (12, 5))
        self.assertTrue(np.all(self.handler.env_values == 0))
        self.assertEqual(self.handler.actual_sample, 5)
        self.assertTrue(self.handler.has_data)

    def test_update_with_valid_line(self):
        line = "1,2,3,4,5,6,7,0,1,0,1,1"
        self.handler.update_data(line)

        np.testing.assert_array_equal(
            self.handler.env_values[:, -1],
            np.array([1,2,3,4,5,6,7,0,1,0,1,1], dtype=float)
        )

    def test_update_with_invalid_length(self):
        before = self.handler.env_values.copy()
        self.handler.update_data("1,2,3")
        np.testing.assert_array_equal(self.handler.env_values, before)

    def test_update_with_non_numeric_values(self):
        with self.assertRaises(ValueError):
            self.handler.update_data("a,b,c,d,e,f,g,h,i,j,k,l")

    def test_fifo_behavior_when_full(self):
        for i in range(6):
            line = ",".join(str(i) for _ in range(12))
            self.handler.update_data(line)

        self.assertEqual(self.handler.env_values[0, -1], 5.0)
        self.assertEqual(self.handler.env_values[0, 0], 1.0)

    def test_shift_left(self):
        self.handler.env_values[0] = np.array([1, 2, 3, 4, 5])
        self.handler.shift_left()
        np.testing.assert_array_equal(
            self.handler.env_values[0],
            np.array([2, 3, 4, 5, 5])
        )


if __name__ == "__main__":
    unittest.main()