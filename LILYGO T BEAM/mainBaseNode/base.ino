#include <Arduino.h>
#include <RadioLib.h>

#include "LoRaBoards.h"

#include <ThingsBoard.h>
#include <Arduino_MQTT_Client.h>

// WiFi
const char* ssid = "Cepter";
const char* password = "T0dayIS7";

// ThingsBoard
const char* token = "wftPEzifPcp1Z8szmGCs"; // From the device on ThingsBoard
const char* server = "192.168.1.145";
const uint16_t port = 1883;

// WiFi + MQTT
WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient);

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected!");
}

void connectTB() {
  if (!tb.connected()) {
    Serial.print("Connecting to ThingsBoard...");
    if (!tb.connect(server, token, port)) {
      Serial.println(" failed!");
    } else {
      Serial.println(" connected!");
    }
  }
}

void setup() {
  setupBoards();
  delay(1500);
Serial.begin(115200);
  connectWiFi();
  
}

void loop() {
  connectTB();
  tb.loop();

  // Send dummy telemetry
  Serial.println("Sending dummy data...");
  tb.sendTelemetryData("temperature", 69.0);
  delay(5000);  // Every 5 seconds
}