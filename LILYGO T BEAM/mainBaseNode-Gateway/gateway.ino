#include <Arduino.h>
#include <RadioLib.h>

#include "LoRaBoards.h"

#include <Arduino_MQTT_Client.h>
#include <Server_Side_RPC.h>
#include <Attribute_Request.h>
#include <Shared_Attribute_Update.h> 
#include <ThingsBoard.h>

// Radio Parameters
#define USE_RADIO // If using the lora radio 
#define CONFIG_RADIO_FREQ 850.0
#define CONFIG_RADIO_OUTPUT_POWER 22
#define CONFIG_RADIO_BW 125.0

static volatile bool transmittedFlag = false; // Flag to indicate that a packet was received
static volatile bool receivedFlag = false; // Flag to indicate that a packet was received
void setReceiveFlag(void)
{
  // we got a packet, set the flag
  receivedFlag = true;
}

void setTransmitFlag(void)
{
  // we got a packet, set the flag
  transmittedFlag = true;
}
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

// ####### CONFIGURATION FOR WIFI AND CONNECTION TO THINGSBOARD SHIT ##########

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

// Connects to the virtual devices through the gateway 
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
void RecieveAndSendTelemetry() {
 // !! REPLACE THE RANDOM BULLSHIT VALUES WITH THE TRANSMISSION ADN RECIEVE OF LORA MESSAGES !!


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

// Build JSON payload dynamically using String - probably a better way to do this
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
  // Publish the telemetry data for all the stuff
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
  
  // Checks for setting up the LoRA transmitter
  // initialize radio with default settings
  #ifdef USE_RADIO
    int state = radio.begin();

    printResult(state == RADIOLIB_ERR_NONE);

    Serial.print(F("Radio Initializing ... "));
    if (state == RADIOLIB_ERR_NONE)
    {
      Serial.println(F("success!"));
    }
    else
    {
      Serial.print(F("failed, code "));
      Serial.println(state);
      while (true)
        ;
    }
      // set the function that will be called
  // when packet transmission is finished

    radio.setPacketSentAction(setTransmitFlag);

    radio.setPacketReceivedAction(setReceiveFlag);
  
  // Sets carrier frequency. SX1268/SX1262 : Allowed values are in range from 150.0 to 960.0 MHz.
  if (radio.setFrequency(CONFIG_RADIO_FREQ) == RADIOLIB_ERR_INVALID_FREQUENCY)
  {
    Serial.println(F("Selected frequency is invalid for this module!"));
    while (true)
      ;
  }

  // Sets LoRa link bandwidth. SX1268/SX1262 : Allowed values are 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0 and 500.0 kHz.
  if (radio.setBandwidth(CONFIG_RADIO_BW) == RADIOLIB_ERR_INVALID_BANDWIDTH)
  {
    Serial.println(F("Selected bandwidth is invalid for this module!"));
    while (true)
      ;
  }

  /* Sets LoRa link spreading factor. SX1262:  Allowed values range from 5 to 12. * */
  if (radio.setSpreadingFactor(12) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR)
  {
    Serial.println(F("Selected spreading factor is invalid for this module!"));
    while (true)
      ;
  }

  // Sets LoRa coding rate denominator. SX1278/SX1276/SX1268/SX1262 : Allowed values range from 5 to 8. Only available in LoRa mode.
  if (radio.setCodingRate(6) == RADIOLIB_ERR_INVALID_CODING_RATE)
  {
    Serial.println(F("Selected coding rate is invalid for this module!"));
    while (true)
      ;
  }

  // Sets LoRa sync word. SX1278/SX1276/SX1268/SX1262/SX1280 : Sets LoRa sync word. Only available in LoRa mode.
  if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE)
  {
    Serial.println(F("Unable to set sync word!"));
    while (true)
      ;
  }

  // Sets transmission output power. SX1262 :  Allowed values are in range from -9 to 22 dBm. This method is virtual to allow override from the SX1261 class.
  if (radio.setOutputPower(CONFIG_RADIO_OUTPUT_POWER) ==
      RADIOLIB_ERR_INVALID_OUTPUT_POWER)
  {
    Serial.println(F("Selected output power is invalid for this module!"));
    while (true)
      ;
  }

  Serial.print(F("Passed Radio Setpup ... "));
  #endif

  // * Initialise the MQTT client to communicate with the ThingsBoard server */
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
  // check if it is still connected to teh MQTT broker, if not try to reconnect
  if (!client.connected()) {
    connectToMQTT();
  }
  // Process incoming MQTT messages !!! ENSURE LIMITED BLOCKING SHIT AFTER IT !!!
  client.loop();

  unsigned long now = millis();

  // Send telemetry - this could be swapped out for using the gps time im hoping i can use teh clock time things board to send interval 
  if (now - lastSent > samplingFrequency) {
    RecieveAndSendTelemetry();
    lastSent = now;
  }

  // This is for checking if how often you want the the sensors to be read adn sent to the base station where frequency is 
  // set on the thingsboard dashboard  
  if (now - lastRequest > 30000) {
    requestSharedAttribute();
    lastRequest = now;
  }
}
