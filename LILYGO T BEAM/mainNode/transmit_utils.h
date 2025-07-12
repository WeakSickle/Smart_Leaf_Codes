#ifndef TRANSMIT_UTILS_H
#define TRANSMIT_UTILS_H

#include <Arduino.h>

// Basic structure for holding all the data to be transmitted and processed
struct TRANSMIT_DATA
{
  int ID;
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  float latitude;
  float longitude;
  float altitude;
  uint16_t Temperature;
  uint16_t Moisture;
  uint16_t EC;
  uint16_t PH;
  bool isCharging;
  uint16_t batteryVoltage;
  int batteryPercentage;
};

// Function to format the data into a string for transmission from Node to base node
String FormatMessage(const TRANSMIT_DATA &data) {
  String message = String(data.ID) + "," +
                   String(data.year) + "," +
                   String(data.month) + "," +
                   String(data.day) + "," +
                   String(data.hour) + "," +
                   String(data.minute) + "," +
                   String(data.second) + "," +
                   String(data.latitude, 6) + "," +
                   String(data.longitude, 6) + "," +
                   String(data.altitude, 2) + "," +
                   String(data.Temperature) + "," +
                   String(data.Moisture) + "," +
                   String(data.EC) + "," +
                   String(data.PH) + "," +
                   (data.isCharging ? "1" : "0") + "," +
                   String(data.batteryVoltage) + "," +
                   String(data.batteryPercentage);
  return message;
}

// Function to decode the message from the recieved string to struct (Node to base node)
TRANSMIT_DATA DecodeMessage(const String& message) {
  TRANSMIT_DATA data;
  int index = 0;
  int from = 0;
  int to = 0;

  String parts[17];

  for (int i = 0; i < 17; i++) {
    to = message.indexOf(',', from);
    if (to == -1 && i < 16) break;  // Not enough parts
    if (to == -1) to = message.length(); // Last part

    parts[i] = message.substring(from, to);
    from = to + 1;
  }

  data.ID = parts[0].toInt();
  data.year = parts[1].toInt();
  data.month = parts[2].toInt();
  data.day = parts[3].toInt();
  data.hour = parts[4].toInt();
  data.minute = parts[5].toInt();
  data.second = parts[6].toInt();
  data.latitude = parts[7].toFloat();
  data.longitude = parts[8].toFloat();
  data.altitude = parts[9].toFloat();
  data.Temperature = parts[10].toInt();
  data.Moisture = parts[11].toInt();
  data.EC = parts[12].toInt();
  data.PH = parts[13].toInt();
  data.isCharging = (parts[14] == "1");
  data.batteryVoltage = parts[15].toInt();
  data.batteryPercentage = parts[16].toInt();

  return data;
}



#endif