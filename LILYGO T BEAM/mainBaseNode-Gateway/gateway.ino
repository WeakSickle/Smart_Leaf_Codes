#include <Arduino.h>
#include <RadioLib.h>

#include "LoRaBoards.h"

#include <Arduino_MQTT_Client.h>
#include <Server_Side_RPC.h>
#include <Attribute_Request.h>
#include <Shared_Attribute_Update.h> 
#include <ThingsBoard.h>

// WiFi name and password (will have to change depending on network)
const char* ssid = "Cepter";
const char* password = "T0dayIS7";

// ThingsBoard connection stuff
const char* token = "3g6yOcNlFZj2G9fGQLe8"; // Device access token from ThingsBoard (copied from device page)
const char* server = "192.168.1.145";       // IP address of your local ThingsBoard server IPV4 using ipconfig
const uint16_t port = 1883; // Jus the default port for MQTT


WiFiClient espClient;
PubSubClient client(espClient);

void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void connectToMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("GatewayClient", token, NULL)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}

void connectDevice(const char* deviceName) {
  String payload = String("{\"device\":\"") + deviceName + "\"}";
  client.publish("v1/gateway/connect", payload.c_str());
  Serial.print("Sent connect for ");
  Serial.println(deviceName);
}

void sendTelemetry() {
  // Generate random temps for each device (e.g., 20 to 30 degrees)
  int tempA = random(20, 31);
  int tempB = random(20, 31);
  int tempC = random(20, 31);

  // Fixed values for battery %, latitude and longitude per device
const int batteryA = 75;
const int batteryB = 60;
const int batteryC = 90;

const float latA = 37.7749;
const float longA = -122.4194;

const float latB = 40.7128;
const float longB = -74.0060;

const float latC = 51.5074;
const float longC = -0.1278;

// Build JSON payload dynamically using String
String payload = "{\n";
payload += "  \"Device A\": [{\"temp\": " + String(tempA) 
         + ", \"battery\": " + String(batteryA) 
         + ", \"lat\": " + String(latA, 6) 
         + ", \"long\": " + String(longA, 6) + "}],\n";

payload += "  \"Device B\": [{\"temp\": " + String(tempB) 
         + ", \"battery\": " + String(batteryB) 
         + ", \"lat\": " + String(latB, 6) 
         + ", \"long\": " + String(longB, 6) + "}],\n";

payload += "  \"Device C\": [{\"temp\": " + String(tempC) 
         + ", \"battery\": " + String(batteryC) 
         + ", \"lat\": " + String(latC, 6) 
         + ", \"long\": " + String(longC, 6) + "}]\n";

payload += "}";

  // Publish telemetry and check result - if it fails is probably because the payload is too big
  if (client.publish("v1/gateway/telemetry", payload.c_str())) {
    Serial.println("Telemetry sent successfully.");
    Serial.print("Payload: ");
    Serial.println(payload.length());  // Print the JSON payload
  } else {
    Serial.println("Failed to send telemetry.");
    Serial.print("Payload size: ");
    Serial.println(payload.length());  // Print size of JSON payload
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  setupBoards();
  connectToWiFi();
  client.setServer(server, port);

   // Set MQTT buffer sizes before connecting to broker
  if (!client.setBufferSize(512, 512)) {   // // Increase buffer size to 512 bytes
    Serial.println("Failed to set MQTT buffer sizes!");
  }

  connectToMQTT();

  // Connect each device once at startup
  connectDevice("Device A");
  connectDevice("Device B");
  connectDevice("Device C");
}

unsigned long lastSent = 0;
const unsigned long interval = 5000; // Send every 5 seconds

void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }

  client.loop();

  unsigned long now = millis();
  if (now - lastSent > interval) {
    sendTelemetry();
    lastSent = now;
  }
}
