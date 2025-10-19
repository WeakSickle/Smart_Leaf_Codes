#include <Arduino.h>
#include <Arduino_MQTT_Client.h>
#include <Attribute_Request.h>
#include <NetworkUtils.h>
#include <RadioLib.h>
#include <Server_Side_RPC.h>
#include <Shared_Attribute_Update.h>
#include <ThingsBoard.h>
#include <TransmitUtils.h>

#include "LoRaBoards.h"

// Radio Parameters
#define USE_RADIO  // If using the lora radio
#define CONFIG_RADIO_FREQ 923.20
#define CONFIG_RADIO_OUTPUT_POWER 22
#define CONFIG_RADIO_BW 125.0
#define NUMBER_OF_DEVICES 1  // Intiial Number of devvices (this is adjusted by dash)

// ########## CONFIGURATION FOR WIFI AND CONNECTION TO THINGSBOARD and SAMS NODES #######################
// configuration settings ########## Please read the instruction manual for
// configuring to work WiFi name and password (will have to change depending on
// network)
const char *ssid = "Cepter";
const char *password = "T0dayIS7";

// ThingsBoard connection stuff
const char *token = "3g6yOcNlFZj2G9fGQLe8";  // Device access token from ThingsBoard (copied
                             // from device page)
const char *server = "192.168.1.145";  // IP address of your local ThingsBoard
                                       // server IPV4 using ipconfig



// ########################################################################

const uint16_t port = 1883;  // Just the default port for MQTT
int numberOfDevices = NUMBER_OF_DEVICES;

// List of frequencies currently supported
// float Frequencies[] = {923.2, 923.4, 923.6, 923.8, 924.0,
//                        924.2, 924.4, 924.6, 924.8};
float *Frequencies;
unsigned long *deviceLastReceived;  // Array to hold the last received time for each device
static volatile bool transmittedFlag =false;  // Flag to indicate that a packet was received
static volatile bool receivedFlag = false;  // Flag to indicate that a packet was received
void setReceiveFlag(void) {
  // we got a packet, set the flag
  receivedFlag = true;
}

void setTransmitFlag(void) {
  // we got a packet, set the flag
  transmittedFlag = true;
}
SX1262 radio =
    new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

static String msgReceived = "0";
WiFiClient espClient;
Arduino_MQTT_Client client(espClient);

// Default sampling interval
unsigned long samplingFrequency = 5000;
unsigned long lastSent = 0;
unsigned long lastRequest = 0;
TRANSMIT_DATA *devices = nullptr;

// Function for checking the RADIO setup error codes
bool checkError(int errCode, const char *errMsg);
void RecieveAndSendTelemetry();
// Callback function to handle incoming MQTT messages after request is made
void mqttCallback(char *topic, byte *payload, unsigned int length);
void requestSharedAttribute();
// Sends a request for the shared attribute "samplingFrequency"

void setup() {
  Serial.begin(115200);
  delay(1000);
  // lora board setup and connect to wifi
  setupBoards();
  connectToWiFi(ssid, password);

// Checks for setting up the LoRA transmitter
// initialize radio with default settings
#ifdef USE_RADIO
  int state = radio.begin();

  printResult(state == RADIOLIB_ERR_NONE);

  Serial.print(F("Radio Initializing ... "));
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
// Check and set the radio parameters are good
  radio.setPacketSentAction(setTransmitFlag);
  radio.setPacketReceivedAction(setReceiveFlag);

  checkError(radio.setFrequency(CONFIG_RADIO_FREQ),
             "Selected frequency is invalid for this module!");
  checkError(radio.setBandwidth(CONFIG_RADIO_BW),
             "Selected bandwidth is invalid for this module!");
  checkError(radio.setSpreadingFactor(12),
             "Selected spreading factor is invalid for this module!");
  checkError(radio.setCodingRate(6),
             "Selected coding rate is invalid for this module!");
  checkError(radio.setSyncWord(0xAB), "Unable to set sync word!");
  checkError(radio.setOutputPower(CONFIG_RADIO_OUTPUT_POWER),
             "Selected output power is invalid for this module!");
  Serial.print(F("Passed Radio Setpup ... "));
#endif

  Frequencies = new float[NUMBER_OF_DEVICES];
  deviceLastReceived = new unsigned long[NUMBER_OF_DEVICES];
  float startFreq = 923.2;  // starting frequency
  float increment = 0.2;    // increment for each device

  for (int i = 0; i < NUMBER_OF_DEVICES; i++) {
    Frequencies[i] = startFreq + i * increment;
    deviceLastReceived[i] = 0;
    Serial.println(Frequencies[i]);
  }

  devices = new TRANSMIT_DATA[numberOfDevices];

  // * Initialise the MQTT client to communicate with the ThingsBoard server */
  client.set_server(server, port);
  client.set_data_callback(mqttCallback);
  // Set MQTT buffer sizes before connecting to broker
  if (!client.set_buffer_size(4096,
                              4096)) {  // // Increase buffer size to 4096 bytes
    Serial.println("Failed to set MQTT buffer sizes!");
  }
  // Connect to ThingsBoard MQTT broker
  connectToMQTT(client, token);
  delay(1000);
  // Request the shared attributes from the thingsboard
  requestSharedAttribute();
}

void loop() {
  // Reconnect if MQTT client disconnected
  if (!client.connected()) {
    connectToMQTT(client, token);
  }

  // Process incoming MQTT messages
  client.loop();

  // Request shared attributes periodically
  if (millis() - lastRequest > 30000) {  // every 30 seconds
    requestSharedAttribute();
    lastRequest = millis();
  }


  // Update telemetry at the configured sampling interval
  if (millis() - lastSent > samplingFrequency) {
    // RecieveAndSendTelemetry();
    SendExampleTelemetry();
    lastSent = millis();
  }


  // Check device activity periodically
  static unsigned long lastActivityCheck = 0;
  if (millis() - lastActivityCheck > 60000) { // every 1 minute
    checkDeviceActivity();
    lastActivityCheck = millis();
  }
}

// Function for checking the RADIO setup error codes
bool checkError(int errCode, const char *errMsg) {
  if (errCode != RADIOLIB_ERR_NONE) {
    Serial.println(errMsg);
    while (true);
    return false;  // Will never reach here but good style
  }
  return true;
}

// Main driver function for processing incoming MQTT messages from the dashboard
void mqttCallback(char *topic, byte *payload, unsigned int length) {
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
      // This is more used for testing the attribute but have kept it here incase
      // needed later
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
      // Check for number of devices change attribute
      if (shared.containsKey("numberOfDevices")) {
        int newCount = shared["numberOfDevices"];
        if (newCount > numberOfDevices) {
          // Connect any new devices
          for (int i = numberOfDevices; i < newCount; i++) {
            String deviceName = "SAMS Node " + String(i + 1);
            connectDevice(client, deviceName.c_str());
            // Attribute update message
            setDeviceAttribute(deviceName.c_str(), "active", "true");
          }

        } else if (newCount < numberOfDevices) {
          // Update the status on dashboard to say they are disconnected
          Serial.println("[MQTT] Disconnecting removed devices...");
          for (int i = newCount; i < numberOfDevices; i++) {
            String deviceName = "SAMS Node " + String(i + 1);
            setDeviceAttribute(deviceName.c_str(), "active", "false");
            Serial.print("[MQTT] Disconnected device: ");
            Serial.println(deviceName);
          }
        }

        Serial.print("[MQTT] Number of devices changed: ");
        Serial.println(newCount);

        // Resize Frequencies array based on number of devices 
        delete[] Frequencies;
        Frequencies = new float[newCount];
        float startFreq = 923.2;
        float increment = 0.2;
        for (int i = 0; i < newCount; i++) {
          Frequencies[i] = startFreq + i * increment;
          Serial.println(Frequencies[i]);
        }

        // Resize devices array
        delete[] devices;
        devices = new TRANSMIT_DATA[newCount];

        // Finally update the count
        numberOfDevices = newCount;
      }
    }
  }
}

// Main function to loop through each device frequency, receive data, and send telemetry to thingsboard
void RecieveAndSendTelemetry() {
  const unsigned long timeout = 10000;
  std::vector<TRANSMIT_DATA> validDevices;  // Only include devices that responded

  for (int i = 0; i < numberOfDevices; i++) {
    float freq = Frequencies[i];
    Serial.print(F("Trying frequency: "));
    Serial.println(freq);

    // if the message has already been received in the previous 2 minutes skip
    if (deviceLastReceived[i] != 0 && (millis() - deviceLastReceived[i]) < 120000) {
      Serial.println(F("Message recently received, skipping..."));
      continue;
    }

    // Set frequency
    if (radio.setFrequency(freq) != RADIOLIB_ERR_NONE) {
      Serial.println(F("Failed to set frequency."));
      continue;
    }

    // Start listening
    radio.startReceive();
    unsigned long startTime = millis();
    bool received = false;

    while (!received) {
      if (receivedFlag) {
        receivedFlag = false;
        int state = radio.readData(msgReceived);
        radio.standby();

        if (state == RADIOLIB_ERR_NONE) {
          Serial.println(F("Packet received!"));
          Serial.print(F("Radio Data:\t\t"));
          Serial.println(msgReceived);

          // Add to valid devices
          TRANSMIT_DATA decoded = DecodeMessage(msgReceived);
          validDevices.push_back(decoded);
          received = true;
        } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
          Serial.println(F("CRC error!"));
          break;
        } else {
          Serial.print(F("Receive failed, code "));
          Serial.println(state);
          break;
        }
      }

      // Timeout
      if (millis() - startTime > timeout) {
        Serial.println(F("Receive timeout! Skipping this device."));
        radio.standby();
        break;
      }

    }
  }

  // Only send telemetry for valid devices
  if (!validDevices.empty()) {
    String payload = buildPayload(validDevices.data(), validDevices.size());
    const uint8_t *payloadBytes = (const uint8_t *)payload.c_str();
    size_t payloadLength = payload.length();

    if (client.publish("v1/gateway/telemetry", payloadBytes, payloadLength)) {
      Serial.println("Telemetry sent successfully.");
      Serial.print("Payload size: ");
      Serial.println(payloadLength);
    } else {
      Serial.println("Failed to send telemetry.");
      Serial.print("Payload size: ");
      Serial.println(payloadLength);
    }
  } else {
    Serial.println("No valid device data to send.");
  }
}

// Function to request shared attribute from ThingsBoard (when adding more just add to the payload string)
void requestSharedAttribute() {
  String payload = "{\"sharedKeys\":\"samplingFrequency,numberOfDevices\"}";
  client.publish("v1/devices/me/attributes/request/1",
                 (const uint8_t *)payload.c_str(), payload.length());
  Serial.println(
      "[MQTT] Requested shared attribute: samplingFrequency, numberOfDevices");
}

// Used for controlling the attributes (in this case settign active or not)
void setDeviceAttribute(const char *deviceName, const char *attribute, const char *value) {
  StaticJsonDocument<128> doc;
  doc[attribute] = value;

  StaticJsonDocument<256> wrapperDoc;
  wrapperDoc[deviceName] = doc;

  char buffer[256];
  size_t len = serializeJson(wrapperDoc, buffer);
  client.publish("v1/gateway/attributes", (uint8_t *)buffer, len);

  Serial.print("[MQTT] Published attribute for ");
  Serial.println(deviceName);
}

// Function to send example telemetry data for testing
void SendExampleTelemetry() {
  TRANSMIT_DATA *exampleDevices = new TRANSMIT_DATA[numberOfDevices];

  for (int i = 0; i < numberOfDevices; i++) {
    exampleDevices[i].ID = i + 1;
    exampleDevices[i].year = 2025;
    exampleDevices[i].month = 10;
    exampleDevices[i].day = 16;
    exampleDevices[i].hour = 12;
    exampleDevices[i].minute = 30 + i;
    exampleDevices[i].second = random(0, 60);
    exampleDevices[i].waterOne = 50 + random(-2, 3);
    exampleDevices[i].waterTwo = 60 + random(-2, 3);
    exampleDevices[i].latitude = -36.8541 + i * 0.0001 + random(-10, 11) * 0.00001;
    exampleDevices[i].longitude = 174.769 + i * 0.0001 + random(-10, 11) * 0.00001;
    exampleDevices[i].altitude = 10.5 + i * 0.1 + random(-5, 6) * 0.01;
    exampleDevices[i].Temperature = 186 + random(-5, 6); // 18.6°C * 10
    exampleDevices[i].Moisture = 40 + random(-3, 4);
    exampleDevices[i].EC = 1200 + random(-20, 21);
    exampleDevices[i].PH = 7 + random(-1, 2);
    exampleDevices[i].isCharging = random(0, 2);
    exampleDevices[i].batteryVoltage = 3700 + random(-20, 21);
    exampleDevices[i].batteryPercentage = 85 + random(-2, 3);
  }

  String payload = buildPayload(exampleDevices, numberOfDevices);
  const uint8_t *payloadBytes = (const uint8_t *)payload.c_str();
  size_t payloadLength = payload.length();

  if (client.publish("v1/gateway/telemetry", payloadBytes, payloadLength)) {
    Serial.println("Example telemetry sent successfully.");
    Serial.print("Payload size: ");
    Serial.println(payloadLength);
  } else {
    Serial.println("Failed to send example telemetry.");
    Serial.print("Payload size: ");
    Serial.println(payloadLength);
  }

  delete[] exampleDevices;
}

// Function to check device activity and update "active" attribute
void checkDeviceActivity() {
  unsigned long fourHours = 4UL * 60UL * 60UL * 1000UL; 
  unsigned long now = millis();

  for (int i = 0; i < numberOfDevices; i++) {
    String deviceName = "SAMS Node " + String(i + 1);
    if (deviceLastReceived[i] == 0) continue; // Never received, skip

    if (now - deviceLastReceived[i] > fourHours) {
      setDeviceAttribute(deviceName.c_str(), "active", "false");
      Serial.print("[MQTT] Set device offline due to inactivity: ");
      Serial.println(deviceName);
    }
  }
}