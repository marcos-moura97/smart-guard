import json
import queue
import paho.mqtt.client as mqtt
from typing import Optional

class MQTTHandler:
    def __init__(
        self,
        broker: str = "localhost",
        port: int = 1883,
        sub_topic: str = "/ping",        
        pub_topic: Optional[str] = None
    ):
        self.sub_topic = sub_topic
        self.pub_topic = pub_topic
        self.connected = False

        self.last_line = None
        
        try:
            self.client = mqtt.Client()
            self.client.on_connect = self.on_connect
            self.client.on_message = self.on_message
            self.client.connect(broker, port, keepalive=60)
            self.client.loop_start()
        except ConnectionRefusedError as e:
            print(f"Error: {e.strerror}")

    def read_line(self):
        return self.last_line

    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self.connected = True
            print("MQTT connected.")
            client.subscribe(self.sub_topic)
        else:
            print(f"MQTT connection failed: {rc}")

    def on_message(self, client, userdata, msg):
        try:
            payload = msg.payload.decode("utf-8")
            data = json.loads(payload)

            ax, ay, az = data["imu"]["accel"]

            pitch = data["dir"]["pitch"]
            roll = data["dir"]["roll"]

            pitch_f = data["dir_filtered"]["pitch"]
            roll_f = data["dir_filtered"]["roll"]

            free_fall = int(data["events"]["free_fall"])
            impact = int(data["events"]["impact"])
            tilt_change = int(data["events"]["tilt_change"])
            post_fall = int(data["events"]["post_fall_monitor"])
            fall_detected = int(data["events"]["fall_detected"])

            line = (
                f"{ax:.3f},"
                f"{ay:.3f},"
                f"{az:.3f},"
                f"{pitch:.3f},"
                f"{roll:.3f},"
                f"{pitch_f:.3f},"
                f"{roll_f:.3f},"
                f"{free_fall},"
                f"{impact},"
                f"{tilt_change},"
                f"{post_fall},"
                f"{fall_detected}"
            )
            
            self.last_line = line

        except (json.JSONDecodeError, KeyError, IndexError, queue.Full):
            pass

    def read_line(self) -> str:
        try:
            return self.queue.get_nowait()
        except queue.Empty:
            return ""

    def publish_res(self, detected: bool, prob: float) -> None:
        if self.pub_topic is not None:
            payload = {
                    "detected": detected,
                    "probability": prob
                }
            self.client.publish(self.pub_topic, payload=json.dumps(payload), qos=1)