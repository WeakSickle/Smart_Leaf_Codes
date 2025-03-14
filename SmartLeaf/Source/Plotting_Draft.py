import pandas as pd
from matplotlib import pyplot as plt

columns = ["LATITUDE", "LONGITUDE", "TIMESTAMP_UTC", "SENSOR_1_UG_L", "SENSOR_2_UG_L", "SENSOR_3_UG_L", "SENSOR_4_UG_L", "BATTERY_PCT"]
df = pd.read_csv("data.csv", usecols=columns)
print("Contents in csv file:\n", df)
plt.rcParams["figure.figsize"] = [7.00, 3.50]
plt.rcParams["figure.autolayout"] = True

# Convert timestamp to datetime and extract time
df['TIMESTAMP_UTC'] = pd.to_datetime(df['TIMESTAMP_UTC'])
df['TIME'] = df['TIMESTAMP_UTC'].dt.strftime('%H:%M')  # Creates new column with times

plt.plot(df.TIME, df.SENSOR_1_UG_L)
plt.xlabel("Time (24 hour clock)")
plt.ylabel("Moisture (Micro Litres)")
plt.show()

