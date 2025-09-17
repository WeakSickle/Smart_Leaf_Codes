#include "NetworkUtils.h"

void connectToWiFi(const char* ssid, const char* password) {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void connectToMQTT(Arduino_MQTT_Client &client, const char* token) {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("GatewayClient", token, NULL)) {
      Serial.println(" connected.");
      client.subscribe("v1/gateway/attributes");
    } else {
      Serial.print(" failed, retrying...");
      delay(2000);
    }
  }
}

void connectDevice(Arduino_MQTT_Client &client, const char* deviceName) {
  String payloadStr = String("{\"device\":\"") + deviceName + "\"}"; 
  if (client.publish("v1/gateway/connect", (const uint8_t*)payloadStr.c_str(), payloadStr.length())) {
    Serial.print("Connected to ");
    Serial.println(deviceName);
  } else {
    Serial.print("Failed to connect to ");
    Serial.println(deviceName);
  }
}

String buildPayload(TRANSMIT_DATA validDevices[], int numValidDevices) {
    String payload = "{\n";
    for (int i = 0; i < numValidDevices; i++) {
        String deviceName = "SAMS Node " + String(i + 1);
        payload += "  \"" + deviceName + "\": [";
        payload += transmitDataToJson(validDevices[i]);
        payload += "]";
        if (i < numValidDevices - 1) payload += ",\n";
    }
    payload += "\n}";
    return payload;
}