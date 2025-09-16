#ifndef TRANSMIT_UTILS_H
#define TRANSMIT_UTILS_H

#include <Arduino.h>

// Basic structure for holding all the data to be transmitted and processed by the SAMS system
struct TRANSMIT_DATA
{
  int ID;
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t waterOne;
  uint8_t waterTwo;
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

// Function declarations
/**
 * @brief Format a TRANSMIT_DATA struct into a comma-separated String. Primary for SAMS node to transmit.
 * 
 * @param data The TRANSMIT_DATA struct containing all sensor/device values.
 * @return String The formatted message ready for transmission.
 */
String FormatMessage(const TRANSMIT_DATA &data);

/**
 * @brief Decode a comma-separated String message back into a TRANSMIT_DATA struct. Primary for Base node to decode messsage
 * 
 * @param message The String message received (from LoRa, MQTT, etc.).
 * @return TRANSMIT_DATA The populated struct with decoded values.
 */
TRANSMIT_DATA DecodeMessage(const String &message);

/**
 * @brief Convert a TRANSMIT_DATA struct into a JSON String for MQTT/ThingsBoard. Used by the Base Node
 * 
 * @param d The TRANSMIT_DATA struct containing all sensor/device values.
 * @return String A JSON-formatted string representing the data for sending.
 */
String transmitDataToJson(const TRANSMIT_DATA& d);
#endif
