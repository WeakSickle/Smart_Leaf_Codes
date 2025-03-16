import serial.tools.list_ports
import sqlite3, json
import time
import os


class receiveData:
    def __init__(self, com_port="COM10", baud_rate=9600, db_name="sql.db",  timeout=5):
        # List all available ports
        self.ports = serial.tools.list_ports.comports()
        self.portList = []

        for onePort in self.ports:
            self.portList.append(str(onePort))
            print(f"Found port: {str(onePort)}")

        # Set the desired COM port
        self.COM = com_port  # Change this dynamically if necessary
        self.serialInst = serial.Serial()
        self.serialInst.baudrate = baud_rate
        self.serialInst.port = self.COM
        self.serialInst.open()

        # SQLite connection
        self.sqliteConnection = sqlite3.connect(db_name)
        self.cursor = self.sqliteConnection.cursor()

        # Create table if it doesn't exist
        self.cursor.execute('''CREATE TABLE IF NOT EXISTS data (sku TEXT PRIMARY KEY, data TEXT)''')

        self.idFound = False
        self.valuesFound = False

         # Timeout setting (in seconds)
        self.timeout = timeout
        self.last_received_time = time.time()  # Timestamp for last received data

    def obtainData(self):
        receivingData = True
        while receivingData:
            if self.serialInst.in_waiting:
                # Read data from the serial port
                packet = self.serialInst.readline()
                message = packet.decode('utf').strip()  # Decode and strip any leading/trailing whitespace

                self.last_received_time = time.time()

                # Check if 'ID' is in the message
                if "ID" in message:
                    self.idFound = True
                    message_parts = message.split()  # Split the message into parts
                    self.ID = message_parts[2]  # Get the ID from the split message

                # Check if 'Values' is in the message
                if "Values" in message:
                    self.valuesFound = True
                    message_parts = message.split()  # Split the message into parts
                    self.Values = message_parts[2:]  # Get the Values from the split message
                    self.Values = json.dumps(self.Values)  # Convert value into JSON string array

                # If both ID and Values have been found, insert into the database
                if self.idFound and self.valuesFound:
                    self.idFound = False
                    self.valuesFound = False
                    try:
                        # Insert data into the database
                        self.cursor.execute('''INSERT INTO data (sku, data) VALUES (?, ?)''', (self.ID, self.Values))
                        self.sqliteConnection.commit()
                        print(f"Inserted data: ID = {self.ID}, Values = {self.Values}")
                    except sqlite3.Error as e:
                        print(f"Error inserting data: {e}")

            else:
                if time.time() - self.last_received_time > self.timeout:
                    print("No more data received. Stopping.")
                    receivingData = False  # Stop the loop when no data is available for the timeout period
