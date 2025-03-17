import sqlite3
from incomingUpload import receiveData
import json

def main():

    with open('../config.json', 'r') as file:
        config = json.load(file)
    
    baud_rate = config["baudRate"]
    com_port = config["comPort"]
    devices_connected = config["devicesConnected"]

    incomingData = receiveData(com_port, baud_rate, db_name="sql.db",  timeout=5)
    incomingData.obtainData()
    sqliteConnection = sqlite3.connect('sql.db')
    cursor = sqliteConnection.cursor()

    print("Enter Node ID: ")
    chooseID = input()

    cursor.execute("SELECT sku, time, data FROM data WHERE sku = ?", (chooseID,))

    node = cursor.fetchall()
    
    if node:
        for row in node:
            print(f"ID: {row[0]}, Time: {row[1]}, Data: {row[2]}")
    else:
       print("ID NOT FOUND")

if __name__ == "__main__":
    main()