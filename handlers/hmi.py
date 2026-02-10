import tkinter as tk
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.animation as animation
import numpy as np
from datetime import datetime
from typing import Union

import threading
import queue
import time

from .serial_handler import SerialHandler
from .data_handler import DataHandler
from .mqtt_handler import MQTTHandler


class GraphicalInterface:
    def __init__(self, connection_handler: Union[SerialHandler, MQTTHandler], data_handler: DataHandler):
        self.serial_handler = connection_handler
        self.data_handler = data_handler

        self.root = tk.Tk()
        self.root.title("SmartGuard HMI")

        self.left_frame = tk.Frame(self.root)
        self.left_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        self.right_frame = tk.Frame(self.root, padx=10)
        self.right_frame.pack(side=tk.RIGHT, fill=tk.Y)

        # ------------ PLOTS (3 SUBPLOTS) ----------------
        self.fig, self.axs = plt.subplots(3, 1, figsize=(8, 8), sharex=True)
        plt.subplots_adjust(hspace=0.45)
        self.fig.suptitle("SmartGuard", fontsize=16, fontweight="bold")

        # g-magnitude plot
        self.line_g = self.axs[0].plot([], [], color="#ff4444", label="|g|")[0]
        self.axs[0].set_xlim(0, self.data_handler.max_samples - 1)
        self.axs[0].set_ylabel("G mag [g]")
        self.axs[0].set_ylim(0, 4)
        self.axs[0].grid(True)

        # Pitch + Pitch filtered
        self.line_pitch = self.axs[1].plot([], [], color="#5588ff", label="Pitch")[0]
        self.line_pitch_f = self.axs[1].plot([], [], color="#33ff99", label="Pitch Filtered")[0]
        self.axs[1].set_ylabel("Pitch [°]")
        self.axs[1].set_xlim(0, self.data_handler.max_samples - 1)
        self.axs[1].set_ylim(-120, 120)
        self.axs[1].legend()
        self.axs[1].grid(True)

        # Roll + Roll filtered
        self.line_roll = self.axs[2].plot([], [], color="#ffaa33", label="Roll")[0]
        self.line_roll_f = self.axs[2].plot([], [], color="#aa33ff", label="Roll Filtered")[0]
        self.axs[2].set_ylabel("Roll [°]")
        self.axs[2].set_xlim(0, self.data_handler.max_samples - 1)
        self.axs[2].set_ylim(-120, 210)
        self.axs[2].set_xlabel("Samples")
        self.axs[2].legend()
        self.axs[2].grid(True)

        # For updating with FuncAnimation
        self.lines = [self.line_g, self.line_pitch, self.line_pitch_f,
                      self.line_roll, self.line_roll_f]

        # ------------ RIGHT SIDE: STATUS LABELS ----------------
        self.time_label = tk.Label(
            self.right_frame, text="", font=("Arial", 14, "bold"), anchor="center"
        )
        self.time_label.pack(pady=10, fill=tk.X)

        self.event_names = [
            "Free Fall",
            "Impact",
            "Tilt Change",
            "Post-Fall Monitor",
            "Fall Detected"
        ]

        self.event_labels = []
        for name in self.event_names:
            frame = tk.Frame(self.right_frame)
            frame.pack(pady=6, fill=tk.X)
            tk.Label(frame, text=name, font=("Arial", 12, "bold")).pack()
            value_label = tk.Label(frame, text="False", font=("Arial", 14))
            value_label.pack()
            self.event_labels.append(value_label)

        self.canvas = FigureCanvasTkAgg(self.fig, master=self.left_frame)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

        # Thread-safe queue for incoming data
        self.data_queue = queue.Queue(maxsize=200)

        # Start background serial thread
        self.serial_thread = threading.Thread(target=self.serial_reader_thread,
                                              daemon=True)
        self.serial_thread.start()

        self.ani = animation.FuncAnimation(self.fig, self.update, interval=100, blit=True)

    # -----------------------------------------------------------
    def serial_reader_thread(self):
        """Reads serial at high speed without blocking the UI."""
        while True:
            line = self.serial_handler.read_line()

            if line:
                try:
                    self.data_queue.put(line, timeout=0.01)
                except queue.Full:
                    # Drop oldest entries if queue fills
                    try:
                        self.data_queue.get_nowait()
                    except:
                        pass

            # Small sleep prevents 100% CPU
            time.sleep(0.001)


    # -----------------------------------------------------------
    def update_event_label(self, label: tk.Label, value: int):
        """Updates event status label color + text"""
        if value == 1:
            label.config(text="True", fg="green")
        else:
            label.config(text="False", fg="red")

    # -----------------------------------------------------------
    def update(self, _):
        # Pull all available lines from the queue (fastest way)
        while not self.data_queue.empty():
            try:
                line = self.data_queue.get_nowait()
                self.data_handler.update_data(line)
            except queue.Empty:
                break


        now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        self.time_label.config(text=now)

        if not self.data_handler.has_data:
            return self.lines

        # Extract data arrays from the handler
        ax = self.data_handler.env_values[0]
        ay = self.data_handler.env_values[1]
        az = self.data_handler.env_values[2]
        pitch = self.data_handler.env_values[3]
        roll = self.data_handler.env_values[4]
        pitch_f = self.data_handler.env_values[5]
        roll_f = self.data_handler.env_values[6]

        freefall = int(self.data_handler.env_values[7, -1])
        impact = int(self.data_handler.env_values[8, -1])
        tilt = int(self.data_handler.env_values[9, -1])
        postfall = int(self.data_handler.env_values[10, -1])
        detected = int(self.data_handler.env_values[11, -1])

        # ----- Update events on the right side -----
        values = [freefall, impact, tilt, postfall, detected]
        for label, val in zip(self.event_labels, values):
            self.update_event_label(label, val)

        # ----- Calculate g magnitude -----
        g_mag = np.sqrt(ax**2 + ay**2 + az**2)

        # ----- Update lines -----
        x = np.arange(self.data_handler.max_samples)

        self.line_g.set_data(x, g_mag)
        self.line_pitch.set_data(x, pitch)
        self.line_pitch_f.set_data(x, pitch_f)
        self.line_roll.set_data(x, roll)
        self.line_roll_f.set_data(x, roll_f)

        return self.lines

    def run(self):
        self.root.mainloop()
