import sqlite3
import os
from sqlite3 import Error

try:

    sqliteConnection = sqlite3.connect('sql.db')
    cursor = sqliteConnection.cursor()
    print ('DB init')

    query = 'select sqlite_version();'
    cursor.execute(query)

    result = cursor.fetchall()
    print('SQLite Version is {}'.format(result))

    cursor.close()

except sqlite3.Error as error:
    print('Error occured - ', error)

finally:

    if sqliteConnection:
        sqliteConnection.close()
        print('SQLite Connection closed')
