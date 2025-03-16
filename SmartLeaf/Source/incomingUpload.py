import serial.tools.list_ports
import sqlite3, json
import os

# List all available ports
ports = serial.tools.list_ports.comports()
portList = []

for onePort in ports:
    portList.append(str(onePort))
    print(f"Found port: {str(onePort)}")

# Set the desired COM port
COM = "COM10"  # Change this dynamically if necessary

serialInst = serial.Serial()
serialInst.baudrate = 9600
serialInst.port = COM
serialInst.open()

# SQLite connection
sqliteConnection = sqlite3.connect('sql.db')
cursor = sqliteConnection.cursor()

# Create table if it doesn't exist
cursor.execute('''CREATE TABLE IF NOT EXISTS data (sku TEXT PRIMARY KEY, data TEXT)''')

idFound = False
valuesFound = False

while True:
    if serialInst.in_waiting:
        # Read data from the serial port
        packet = serialInst.readline()
        message = packet.decode('utf-8').strip()  # Decode and strip any leading/trailing whitespace

        # Check if 'ID' is in the message
        if "ID" in message:
            idFound = True
            message_parts = message.split()  # Split the message into parts
            ID = message_parts[2]  # Get the ID from the split message
        # Check if 'Values' is in the message
        if "Values" in message:
            valuesFound = True
            message_parts = message.split()  # Split the message into parts
            Values = message_parts[2:]  # Get the Values from the split message
            Values = json.dumps(Values)
        
        # If both ID and Values have been found, insert into the database
        if idFound and valuesFound:
            idFound = False
            valuesFound = False
            try:
                # Insert data into the database
                cursor.execute('''INSERT INTO data (sku, data) VALUES (?, ?)''', (ID, Values))
                sqliteConnection.commit()
                print(f"Inserted data: ID = {ID}, Values = {Values}")
            except sqlite3.Error as e:
                print(f"Error inserting data: {e}")