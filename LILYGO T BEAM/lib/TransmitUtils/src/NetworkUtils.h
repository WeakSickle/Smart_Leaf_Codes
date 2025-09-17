#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include <Arduino.h>
#include <WiFi.h>
#include <Arduino_MQTT_Client.h>
#include "TransmitUtils.h"   // so you can use TRANSMIT_DATA inside

/**
 * @brief Connects the board to a WiFi network.
 * @param ssid The SSID (network name) of the WiFi network.
 * @param password The password for the WiFi network.
 */
void connectToWiFi(const char* ssid, const char* password);

/**
 * @brief Connects to an MQTT broker (ThingsBoard) using a client object and device token.
 * @param client Reference to an Arduino_MQTT_Client instance.
 * @param token Device access token for ThingsBoard authentication.
 */
void connectToMQTT(Arduino_MQTT_Client &client, const char* token);

/**
 * @brief Connects a virtual device through the ThingsBoard gateway.
 * @param client Reference to an Arduino_MQTT_Client instance.
 * @param deviceName Name of the virtual device to connect.
 */
void connectDevice(Arduino_MQTT_Client &client, const char* deviceName);

/**
 * @brief Builds a JSON payload string containing telemetry data from multiple devices.
 * @param devices Array of TRANSMIT_DATA structs containing device data.
 * @param numDevices Number of devices in the array.
 * @return A String containing the JSON payload.
 */
String buildPayload(TRANSMIT_DATA devices[], int numDevices);

#endif
