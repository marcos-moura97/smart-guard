import json
import queue
import paho.mqtt.client as mqtt

class MQTTHandler:
    def __init__(
        self,
        broker: str = "localhost",
        port: int = 1883,
        topic: str = "/ping"
    ):
        self.topic = topic
        self.connected = False

        self.queue = queue.Queue(maxsize=500)
        try:
            self.client = mqtt.Client()
            self.client.on_connect = self.on_connect
            self.client.on_message = self.on_message

            self.client.connect(broker, port, keepalive=60)
            self.client.loop_start()
        except ConnectionRefusedError as e:
            print(f"Error: {e.strerror}")


    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self.connected = True
            print("MQTT connected.")
            client.subscribe(self.topic)
        else:
            print(f"MQTT connection failed: {rc}")


    def on_message(self, client, userdata, msg):
        try:
            payload = msg.payload.decode("utf-8")
            data = json.loads(payload)

            line = (
                f"{data['ax']:.3f},"
                f"{data['ay']:.3f},"
                f"{data['az']:.3f},"
                f"{data['pitch']:.3f},"
                f"{data['roll']:.3f},"
                f"{data['pitch_f']:.3f},"
                f"{data['roll_f']:.3f},"
                f"{int(data['freeFall'])},"
                f"{int(data['impact'])},"
                f"{int(data['tilt'])},"
                f"{int(data['postFall'])},"
                f"{int(data['detected'])}"
            )

            self.queue.put_nowait(line)

        except (json.JSONDecodeError, KeyError, queue.Full):
            pass

    def read_line(self) -> str:
        try:
            return self.queue.get_nowait()
        except queue.Empty:
            return ""
