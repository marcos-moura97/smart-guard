from handlers import SerialHandler, DataHandler, GraphicalInterface

if __name__ == "__main__":
    serial_handler = SerialHandler(port="COM3")
    data_handler = DataHandler(200)
    app = GraphicalInterface(serial_handler, data_handler)
    app.run()