#include "TransmitUtils.h"

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
                   String(data.waterOne) + "," +
                   String(data.waterTwo) + "," +
                   String(data.Temperature) + "," +
                   String(data.Moisture) + "," +
                   String(data.EC) + "," +
                   String(data.PH) + "," +
                   (data.isCharging ? "1" : "0") + "," +
                   String(data.batteryVoltage) + "," +
                   String(data.batteryPercentage);
  return message;
}

// Function to decode the message from the received string to struct
TRANSMIT_DATA DecodeMessage(const String& message) {
  TRANSMIT_DATA data;
  int from = 0;
  int to = 0;

  String parts[19];
  for (int i = 0; i < 19; i++) {
    to = message.indexOf(',', from);
    if (to == -1 && i < 18) break;  // Not enough parts
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
  data.waterOne = parts[10].toInt();
  data.waterTwo = parts[11].toInt();
  data.Temperature = parts[12].toInt();
  data.Moisture = parts[13].toInt();
  data.EC = parts[14].toInt();
  data.PH = parts[15].toInt();
  data.isCharging = (parts[16] == "1");
  data.batteryVoltage = parts[17].toInt();
  data.batteryPercentage = parts[18].toInt();

  return data;
}

// Updated JSON function
String transmitDataToJson(const TRANSMIT_DATA& d) {
    String s = "{";

    s += "\"ID\": " + String(d.ID) + ",";
    s += "\"year\": " + String(d.year) + ",";
    s += "\"month\": " + String(d.month) + ",";
    s += "\"day\": " + String(d.day) + ",";
    s += "\"hour\": " + String(d.hour) + ",";
    s += "\"minute\": " + String(d.minute) + ",";
    s += "\"second\": " + String(d.second) + ",";

    s += "\"latitude\": " + String(d.latitude, 6) + ",";
    s += "\"longitude\": " + String(d.longitude, 6) + ",";
    s += "\"altitude\": " + String(d.altitude, 2) + ",";

    s += "\"waterOne\": " + String(d.waterOne) + ",";
    s += "\"waterTwo\": " + String(d.waterTwo) + ",";

    s += "\"Temperature\": " + String(d.Temperature) + ",";
    s += "\"Moisture\": " + String(d.Moisture) + ",";
    s += "\"EC\": " + String(d.EC) + ",";
    s += "\"PH\": " + String(d.PH) + ",";

    s += "\"isCharging\": " + String(d.isCharging ? "true" : "false") + ",";
    s += "\"batteryVoltage\": " + String(d.batteryVoltage) + ",";
    s += "\"batteryPercentage\": " + String(d.batteryPercentage);

    s += "}";  // close values + main {}
    return s;
}
