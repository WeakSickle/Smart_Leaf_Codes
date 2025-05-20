#include <Arduino.h>
#include <RadioLib.h>

#include "LoRaBoards.h"

#include <Arduino_MQTT_Client.h>
#include <Server_Side_RPC.h>
#include <Attribute_Request.h>
#include <Shared_Attribute_Update.h> 
#include <ThingsBoard.h>
// WiFi
const char* ssid = "Cepter";
const char* password = "T0dayIS7";

// ThingsBoard
const char* token = "wftPEzifPcp1Z8szmGCs"; // Device access token from ThingsBoard
const char* server = "192.168.1.145";       // IP address of your local ThingsBoard server
const uint16_t port = 1883;

constexpr size_t MAX_ATTRIBUTES = 1U;
constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;
Attribute_Request<MAX_ATTRIBUTES> attr_request;
Shared_Attribute_Update<MAX_ATTRIBUTES, MAX_ATTRIBUTES> shared_update;


const std::array<IAPI_Implementation*, 2U> apis = {
    &attr_request,
    &shared_update
};

// WiFi + MQTT
WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient,Default_Payload_Size,Default_Payload_Size,Default_Max_Stack_Size,apis);



constexpr uint64_t REQUEST_TIMEOUT_MICROSECONDS = 5000U * 1000U;

// Attribute names for attribute request and attribute updates functionality

constexpr const char SAMPLING_ATTR[] = "samplingFrequency";

// Default sampling frequency in milliseconds
int samplingFrequency = 5000;



// Process shared attribute updates
void processSharedAttributes(const JsonObjectConst &data) {
  for (auto it = data.begin(); it != data.end(); ++it) {
    if (strcmp(it->key().c_str(), SAMPLING_ATTR) == 0) {
      int newVal = it->value().as<int>();
      if (newVal > 0) {
        samplingFrequency = newVal;
        Serial.print("Updated samplingFrequency to ");
        Serial.println(samplingFrequency);
      }
    }
  }
}

const Shared_Attribute_Callback<MAX_ATTRIBUTES> sharedAttributesCallback(&processSharedAttributes);
const Attribute_Request_Callback<MAX_ATTRIBUTES> attrRequestCallback(&processSharedAttributes);



void subscribeAndRequestAttributes() {
  if (!shared_update.Shared_Attributes_Subscribe(sharedAttributesCallback)) {
    Serial.println("Failed to subscribe to shared attributes");
  }
  if (!attr_request.Shared_Attributes_Request(attrRequestCallback)) {
    Serial.println("Failed to request shared attributes");
  }
}


// Connect to WiFi
void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected!");
}

// Connect to ThingsBoard and subscribe to shared attribute updates
void connectTB() {
  if (!tb.connected()) {
    Serial.print("Connecting to ThingsBoard...");
    if (!tb.connect(server, token, port)) {
      Serial.println(" failed!");
    } else {
      Serial.println(" connected!");
      subscribeAndRequestAttributes();
    }
  }
}

void setup() {
  setupBoards();  // Your board-specific initialization
  delay(1500);
  Serial.begin(115200);
  connectWiFi();
}

void loop() {
  connectTB();
  tb.loop();
  // Send telemetry data at the specified sampling frequency
 static unsigned long lastSend = 0;
  unsigned long now = millis();

  // Send telemetry based on samplingFrequency, but check frequently to keep MQTT alive
  if (now - lastSend >= (unsigned long)samplingFrequency) {
    Serial.println("Sending telemetry...");
    tb.sendTelemetryData("temperature", 69.0);
    lastSend = now;
  }
  delay(10);
}
