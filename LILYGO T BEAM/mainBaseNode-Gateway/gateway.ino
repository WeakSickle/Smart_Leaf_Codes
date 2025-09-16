#include <Arduino.h>
#include <RadioLib.h>

#include "LoRaBoards.h"

#include <Arduino_MQTT_Client.h>
#include <Server_Side_RPC.h>
#include <Attribute_Request.h>
#include <Shared_Attribute_Update.h> 
#include <ThingsBoard.h>
#include "transmit_utils.h"

// Radio Parameters
#define USE_RADIO // If using the lora radio 
#define CONFIG_RADIO_FREQ 850.0
#define CONFIG_RADIO_OUTPUT_POWER 22
#define CONFIG_RADIO_BW 125.0
#define NUMBER_OF_DEVICES 1 // Number of devices in use that will be connected to the base node

// float Frequencies[] = {923.2, 923.4, 923.6, 923.8, 924.0,
//                        924.2, 924.4, 924.6, 924.8};
float Frequencies[] = {923.2};

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
const char* ssid = "SmartLeaf";
const char* password = "qzan8545";

// ThingsBoard connection stuff
const char* token = "3g6yOcNlFZj2G9fGQLe8"; // Device access token from ThingsBoard (copied from device page)
const char* server = "10.209.97.110";       // IP address of your local ThingsBoard server IPV4 using ipconfig
const uint16_t port = 1883; // Jus the default port for MQTT

static String msgReceived = "0";
WiFiClient espClient;
Arduino_MQTT_Client client(espClient);

// Default sampling interval
unsigned long samplingFrequency = 5000;
unsigned long lastSent = 0;
unsigned long lastRequest = 0;
TRANSMIT_DATA devices[NUMBER_OF_DEVICES]; // Structs for each of the number of devices


// Function for checking the RADIO setup error codes 
bool checkError(int errCode, const char* errMsg) {
  if (errCode != RADIOLIB_ERR_NONE) {
    Serial.println(errMsg);
    while (true);
    return false;  // Will never reach here but good style
  }
  return true;
}

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

void ConnectToDevices() {
  char deviceName[20];  // adjust size if names might be longer

  for (int i = 0; i < NUMBER_OF_DEVICES; i++) {
    snprintf(deviceName, sizeof(deviceName), "SAMS Node %d", i + 1);
    connectDevice(deviceName);   // safe: deviceName[] stays alive
    delay(1000);
  }
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
    Serial.print("Connected to ");
    Serial.println(deviceName);
  } else {
    Serial.print("Failed to connect to ");
    Serial.println(deviceName);
  }
}

// Function to combine multiple devices data into Payload 
String buildPayload() {
    String payload = "{\n";

    for (int i = 0; i < NUMBER_OF_DEVICES; i++) {
        String deviceName = "SAMS Node " + String(i + 1);

        payload += "  \"" + deviceName + "\": [";
        payload += transmitDataToJson(devices[i]);
        payload += "]";

        if (i < NUMBER_OF_DEVICES - 1) {
            payload += ",\n";
        } else {
            payload += "\n";
        }
    }

    payload += "}";

    return payload;
}

// This is where potentially the readings of are formated from lora and sent
void RecieveAndSendTelemetry() {
 // !! REPLACE THE RANDOM BULLSHIT VALUES WITH THE TRANSMISSION ADN RECIEVE OF LORA MESSAGES !!
  for (int i = 0; i < sizeof(Frequencies) / sizeof(Frequencies[0]); i++) {
    float freq = Frequencies[i];
    Serial.print(F("Trying frequency: "));
    Serial.println(freq);

    // Set frequency
    if (radio.setFrequency(freq) != RADIOLIB_ERR_NONE) {
      Serial.println(F("Failed to set frequency."));
      continue;
    }

    // listen for a packet for a while on this frequency
    radio.startReceive();  // Or blocking: radio.receive(NULL);
    while (!receivedFlag) {
      // idle, waiting for interrupt/callback to set receivedFlag
      delay(10); // Prevents watchdog reset or tight loop hogging CPU
    }

    // Process the received packet
    receivedFlag = false;
    int state = radio.readData(msgReceived);
    // Stop listening before changing frequency
    radio.standby();
    flashLed();

    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("Packet received!"));
      Serial.print(F("Radio Data:\t\t"));
      Serial.println(msgReceived);
      devices[i] = DecodeMessage(msgReceived);
    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      Serial.println(F("CRC error!"));
    } else {
      Serial.print(F("Receive failed, code "));
      Serial.println(state);
    }
  }
  // // Cycle through each frequency device
  // for (int i = 0; i < NUMBER_OF_DEVICES; i++) {
  //   // Simulate receiving data from each device

  //   devices[i].ID = i + 1; // Device ID
  //   devices[i].year = 2023;
  //   devices[i].month = 10;
  //   devices[i].day = 1;
  //   devices[i].hour = 12;
  //   devices[i].minute = 0;
  //   devices[i].second = 0;
  //   devices[i].latitude = 37.7749 + (i * 0.01); // Simulated latitude
  //   devices[i].longitude = -122.4194 + (i * 0.01); // Simulated longitude
  //   devices[i].altitude = 10.0 + (i * 5); // Simulated altitude
  //   devices[i].Temperature = random(20, 30); // Simulated temperature
  //   devices[i].Moisture = random(30, 70); // Simulated moisture
  //   devices[i].EC = random(100, 500); // Simulated EC
  //   devices[i].PH = random(6, 8); // Simulated pH
  //   devices[i].isCharging = (i % 2 == 0); // Charging status alternating
  //   devices[i].batteryVoltage = random(3, 5); // Simulated battery voltage
  //   devices[i].batteryPercentage = random(50, 100); // Simulated battery percentage
  // }

  // Build the payload from all devices
  String payload = buildPayload();

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
  
    checkError(radio.setFrequency(CONFIG_RADIO_FREQ), "Selected frequency is invalid for this module!");
    checkError(radio.setBandwidth(CONFIG_RADIO_BW), "Selected bandwidth is invalid for this module!");
    checkError(radio.setSpreadingFactor(12), "Selected spreading factor is invalid for this module!");
    checkError(radio.setCodingRate(6), "Selected coding rate is invalid for this module!");
    checkError(radio.setSyncWord(0xAB), "Unable to set sync word!");
    checkError(radio.setOutputPower(CONFIG_RADIO_OUTPUT_POWER), "Selected output power is invalid for this module!");
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
  delay(1000);
  // requestSharedAttribute();

  // Connect each device once at startup
  // ConnectToDevices();
  connectDevice("SAMS Node 1");
  // connectDevice("Device B");
  // connectDevice("Device C");
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
  // if (now - lastRequest > 30000) {
  //   requestSharedAttribute();
  //   lastRequest = now;
  // }
}
