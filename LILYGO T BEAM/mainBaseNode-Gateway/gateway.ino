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
Arduino_MQTT_Client client(espClient);

// Default sampling interval
unsigned long samplingFrequency = 5000;
unsigned long lastSent = 0;
unsigned long lastRequest = 0;

// Connect to the wifi
void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

// Connection to MQTT broker
void connectToMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("GatewayClient", token, NULL)) {
      Serial.println(" connected.");
      if (client.subscribe("v1/gateway/attributes")) {
        Serial.println("Subscribed to v1/gateway/attributes");
      } else {
        Serial.println("Failed to subscribe to v1/gateway/attributes");
      }
    } else {
      Serial.print(" failed, rc=");
      Serial.println(" retrying in 2 seconds");
      delay(2000);
    }
  }
}

// Callback function to handle incoming MQTT messages after request is made
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("\n[MQTT] Topic: ");
  Serial.println(topic);

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    Serial.println("[MQTT] Failed to parse JSON");
    return;
  }

  // Check for gateway shared attribute response
  if (String(topic).startsWith("v1/devices/me/attributes/response/")) {
    if (doc.containsKey("shared")) {
      JsonObject shared = doc["shared"];
      if (shared.containsKey("samplingFrequency")) {
        // Check if the value is differnt from the current one
        if (shared["samplingFrequency"] != samplingFrequency) {
          // Update the sampling frequency
          samplingFrequency = shared["samplingFrequency"];
          Serial.print("[MQTT] Updated samplingFrequency to: ");
          Serial.println(samplingFrequency);
        } else {
          Serial.println("[MQTT] No change in samplingFrequency");
        }
      } else {
        Serial.println("[MQTT] samplingFrequency not found in response");
      }
    }
  }
}

// Sends a request for the shared attribute "samplingFrequency" 
void requestSharedAttribute() {
  String payload = "{\"sharedKeys\":\"samplingFrequency\"}";
  client.publish("v1/devices/me/attributes/request/1", (const uint8_t*)payload.c_str(), payload.length());
  Serial.println("[MQTT] Requested shared attribute: samplingFrequency");
}



// Connects to teh virtual devices through the gateway 
void connectDevice(const char* deviceName) {
  String payloadStr = String("{\"device\":\"") + deviceName + "\"";
  const uint8_t* payload = (const uint8_t*)payloadStr.c_str();
  size_t length = payloadStr.length();

  if (client.publish("v1/gateway/connect", payload, length)) {
    Serial.print("Sent connect for ");
    Serial.println(deviceName);
  } else {
    Serial.print("Failed to connect device: ");
    Serial.println(deviceName);
  }
}

// This is where potentially the readings of are formated from lora and sent
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

   const uint8_t* payloadBytes = (const uint8_t*)payload.c_str();
  size_t payloadLength = payload.length();
  // Publish the telemetry data for all teh stuff
  if (client.publish("v1/gateway/telemetry", payloadBytes, payloadLength)) {
    Serial.println("Telemetry sent successfully.");
    Serial.print("Payload size: ");
    Serial.println(payloadLength);
  } else {
    Serial.println("Failed to send telemetry.");
    Serial.print("Payload size: ");
    Serial.println(payloadLength);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  setupBoards();
  connectToWiFi();
  
  client.set_server(server, port);
  client.set_data_callback(mqttCallback);
  // Set MQTT buffer sizes before connecting to broker
  if (!client.set_buffer_size(512, 512)) {   // // Increase buffer size to 512 bytes
    Serial.println("Failed to set MQTT buffer sizes!");
  }

  connectToMQTT();
  requestSharedAttribute();

  // Connect each device once at startup
  connectDevice("Device A");
  connectDevice("Device B");
  connectDevice("Device C");
}

void loop() {
    if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();

  unsigned long now = millis();

  // Send telemetry
  if (now - lastSent > samplingFrequency) {
    sendTelemetry();
    lastSent = now;
  }

  // Re-request attribute every 30 seconds
  if (now - lastRequest > 30000) {
    requestSharedAttribute();
    lastRequest = now;
  }
}
