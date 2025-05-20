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
const char* token = "wftPEzifPcp1Z8szmGCs"; // Device access token from ThingsBoard (copied from device page)
const char* server = "192.168.1.145";       // IP address of your local ThingsBoard server IPV4 using ipconfig
const uint16_t port = 1883; // Jus the default port for MQTT


constexpr size_t MAX_ATTRIBUTES = 1U;
constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;

// Create instances of the ThingsBoard API classes
Attribute_Request<MAX_ATTRIBUTES> attr_request;
Shared_Attribute_Update<MAX_ATTRIBUTES, MAX_ATTRIBUTES> shared_update;

// Can include the rpc method here if needed for one off commands 
const std::array<IAPI_Implementation*, 2U> apis = {
    &attr_request,
    &shared_update
};

// WiFi + MQTT
#define Buffer_size 256 // Custom size fo rthe message buffer to ensure message can be sent - if throws error increase 
// (default buffer is to small)

WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient,Buffer_size,Buffer_size,Default_Max_Stack_Size,apis);

bool subscribed = false;

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

void connectTB() {
  if (!tb.connected()) {
    Serial.print("Connecting to ThingsBoard...");
    if (!tb.connect(server, token, port)) {
      Serial.println(" failed!");
      subscribed = false;  // Reset subscription state if failed
    } else {
      Serial.println(" connected!");
      if (!subscribed) {
        subscribeAndRequestAttributes();
        subscribed = true;  // Mark subscribed to avoid duplicate subscriptions
      }
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
    // Reconnect if not connected anymore 
   if (!tb.connected()) {
    connectTB();
  }
  tb.loop();
  // Send telemetry data at the specified sampling frequency
    static unsigned long lastSend = 0;
  unsigned long now = millis();

  // Send telemetry based on samplingFrequency, but check frequently to keep MQTT alive
  if (now - lastSend >= (unsigned long)samplingFrequency) {
    
    Serial.println("Sending telemetry...");
    // Prepare telemetry payload with fake data
    StaticJsonDocument<256> doc;

    // Fake sensor values
    doc["sensor1"] = random(100, 201);
    doc["sensor2"] = random(100, 201);   
    doc["sensor3"] = random(100, 201);   
    doc["sensor4"] = random(100, 201); 

  // Fake battery info
  doc["batteryVoltage"] = 3.85;
  doc["batteryLevel"] = 74;

  // Fake GPS location
  doc["latitude"] = -36.8485;
  doc["longitude"] = 174.7633;
  doc["altitude"] = 12.5;

  // Send all as a single JSON payload
  tb.sendTelemetryJson(doc, measureJson(doc));
    
    // tb.sendTelemetryData("temperature", 69.0);
    lastSend = now;
  }
  delay(10);
}
