import sqlite3

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

sqliteConnection.close()
