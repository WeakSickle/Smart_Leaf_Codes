#include <Arduino.h>

#include "LoRaBoards.h"
#include "RadioLib.h"

#define CONFIG_RADIO_OUTPUT_POWER 22
#define CONFIG_RADIO_BW 125.0 // Note that the carrier seperation is 200 so doesnt overlap with other
                              // freq
// Define the Range of carrier frequencies to sweep over
float Frequencies[] = {923.2, 923.4, 923.6, 923.8, 924.0,
                       924.2, 924.4, 924.6, 924.8};
int FrequencyIndex = 0; 
SX1262 radio =
    new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

// flag to indicate that a packet was received
static volatile bool SentFlag, receivedFlag = false;
static String payload = "0";
static String ackPayload = "0";

void setFlag(void)
{
  // set the flag
  SentFlag = true;
}

void setRecieveFlag(void)
{
  // set the flag
  receivedFlag = true;
}

// SCRIPT TO BE FOR TESTING BASE NODE RECEIVING OVER MULTIPLCE CARRIER
// FREQUENCIES

void setup()
{
  setupBoards(); // Setup the Board (pretty sure its the pins)
  delay(1500);
  Serial.begin(115200);
  // initialize radio with default settings
  int state = radio.begin();

  radio.setPacketSentAction(setFlag);

  // SX1262 Configuration

  // Frequency (valid range for SX1262: 150–960 MHz; NZ ISM: 923–925 MHz)
  if (radio.setFrequency(Frequencies[FrequencyIndex]) == RADIOLIB_ERR_INVALID_FREQUENCY)
  {
    Serial.println(F("Invalid frequency!"));
    while (true)
      ;
  }

  // Bandwidth (valid: 7.8–500 kHz)
  if (radio.setBandwidth(CONFIG_RADIO_BW) == RADIOLIB_ERR_INVALID_BANDWIDTH)
  {
    Serial.println(F("Invalid bandwidth!"));
    while (true)
      ;
  }

  // Spreading Factor (valid: 5–12)
  if (radio.setSpreadingFactor(12) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR)
  {
    Serial.println(F("Invalid spreading factor!"));
    while (true)
      ;
  }

  // Coding Rate (valid: 5–8, representing 4/5 to 4/8)
  if (radio.setCodingRate(6) == RADIOLIB_ERR_INVALID_CODING_RATE)
  {
    Serial.println(F("Invalid coding rate!"));
    while (true)
      ;
  }

  // Sync Word (optional, use same value for all devices)
  if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE)
  {
    Serial.println(F("Failed to set sync word!"));
    while (true)
      ;
  }

  // Output Power (-9 to +22 dBm for SX1262)
  if (radio.setOutputPower(CONFIG_RADIO_OUTPUT_POWER) ==
      RADIOLIB_ERR_INVALID_OUTPUT_POWER)
  {
    Serial.println(F("Invalid output power!"));
    while (true)
      ;
  }

  // Overcurrent Protection Limit (45–240 mA)
  if (radio.setCurrentLimit(140) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT)
  {
    Serial.println(F("Invalid current limit!"));
    while (true)
      ;
  }

  // Preamble Length (1–65535 symbols)
  if (radio.setPreambleLength(16) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH)
  {
    Serial.println(F("Invalid preamble length!"));
    while (true)
      ;
  }

  // CRC (enable/disable)
  if (radio.setCRC(false) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION)
  {
    Serial.println(F("Invalid CRC setting!"));
    while (true)
      ;
  }

  radio.setPacketReceivedAction(setRecieveFlag);
}

void loop()
{
  Serial.print(F("Transmitting on frequency: "));
  Serial.println(Frequencies[FrequencyIndex]);

  bool acknowledged = false;
  unsigned long startTime = millis();

  while (!acknowledged && millis() - startTime < 10000)
  { // 10 sec max
    String message = "Hello from sender";
    int state = radio.transmit(message);

    if (state == RADIOLIB_ERR_NONE)
    {
      Serial.println(F("Transmission successful!"));
      Serial.print(F("Time on air: "));
      Serial.println(radio.getTimeOnAir(message.length()));
    }
    else
    {
      Serial.print(F("Transmission failed, code "));
      Serial.println(state);
      while (state != RADIOLIB_ERR_NONE)
      {
        // Retry transmission until success
        state = radio.transmit(message);
        delay(100);
      }
    }
    flashLed();

    delay(100); // JUST INCASE IT DOENST LIKE SWITCHING QUICKLY IDK

    // Listen for ACK
    radio.startReceive();
    unsigned long waitStart = millis();
    const unsigned long waitForAck = 5000;

    while (!receivedFlag && millis() - waitStart < waitForAck)
    {
      delay(10); // Avoid tight loop
    }

    if (receivedFlag)
    {
      receivedFlag = false;
      ackPayload = "0";
      int ackState = radio.readData(ackPayload);

      if (ackState == RADIOLIB_ERR_NONE)
      {
        Serial.print(F("Received: "));
        Serial.println(ackPayload);

        if (String(ackPayload) == "ACK")
        {
          Serial.println(F("ACK received. Stopping transmission."));
          acknowledged = true;
        }
      }
      else
      {
        Serial.print(F("ACK receive failed, code "));
        Serial.println(ackState);
      }
    }

    delay(500); // Probably need to adjsut this
  }

  radio.standby();
}
