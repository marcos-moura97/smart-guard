from handlers import SerialHandler, MQTTHandler, DataHandler, GraphicalInterface

if __name__ == "__main__":
    #serial_handler = SerialHandler(port="COM3")
    mqtt_handler = MQTTHandler(
        broker="192.168.1.14",
        port=1883,
        topic="/data"
    )
    data_handler = DataHandler(200)
    app = GraphicalInterface(mqtt_handler, data_handler)
    app.run()