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

    cursor.execute("SELECT sku, data FROM data WHERE sku = ?", (chooseID,))

    node = cursor.fetchall()

    if node:
       print(f"ID : {node[0][0]}, Data: {node[0][1]}")
    else:
       print("ID NOT FOUND")

if __name__ == "__main__":
    main()