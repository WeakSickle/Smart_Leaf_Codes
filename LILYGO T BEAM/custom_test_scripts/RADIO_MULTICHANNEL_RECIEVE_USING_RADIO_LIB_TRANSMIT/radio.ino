#include <Arduino.h>
#include "LoRaBoards.h"
#include "RadioLib.h"

#define CONFIG_RADIO_OUTPUT_POWER 22
#define CONFIG_RADIO_BW 125.0

float Frequencies[] = {923.2, 923.4, 923.6, 923.8, 924.0,
                       924.2, 924.4, 924.6, 924.8};

Module module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
SX1262 radio(&module);

static volatile bool receivedFlag = false;
static String payload = "0";
static String rssi = "0dBm";
static String snr = "0dB";

void setFlag(void) {
  receivedFlag = true;
}




void setup() {
  setupBoards();
  delay(1500);
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  if (radio.begin() != RADIOLIB_ERR_NONE) {
    Serial.println(F("Radio init failed!"));
    while (true);
  }

  radio.setPacketReceivedAction(setFlag);

  if (radio.setBandwidth(CONFIG_RADIO_BW) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
    Serial.println(F("Invalid bandwidth!"));
    while (true);
  }

  if (radio.setSpreadingFactor(12) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
    Serial.println(F("Invalid spreading factor!"));
    while (true);
  }

  if (radio.setCodingRate(6) == RADIOLIB_ERR_INVALID_CODING_RATE) {
    Serial.println(F("Invalid coding rate!"));
    while (true);
  }

  if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE) {
    Serial.println(F("Failed to set sync word!"));
    while (true);
  }

  if (radio.setOutputPower(CONFIG_RADIO_OUTPUT_POWER) ==
      RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
    Serial.println(F("Invalid output power!"));
    while (true);
  }

  if (radio.setCurrentLimit(140) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
    Serial.println(F("Invalid current limit!"));
    while (true);
  }

  if (radio.setPreambleLength(16) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
    Serial.println(F("Invalid preamble length!"));
    while (true);
  }

  if (radio.setCRC(false) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
    Serial.println(F("Invalid CRC config!"));
    while (true);
  }

  Serial.println(F("Receiver ready."));
  drawMain();
}

void loop() {
  for (int i = 0; i < sizeof(Frequencies) / sizeof(Frequencies[0]); i++) {
    float freq = Frequencies[i];
    Serial.print(F("Listening on frequency: "));
    Serial.println(freq);

    if (radio.setFrequency(freq) != RADIOLIB_ERR_NONE) {
      Serial.println(F("Failed to set frequency."));
      continue;
    }

    receivedFlag = false;
    radio.startReceive();

    unsigned long start = millis();
    while (!receivedFlag && millis() - start < 5000) {
      delay(10);  // brief idle while waiting for interrupt
    }

    if (!receivedFlag) {
      Serial.println(F("No packet received."));
      radio.standby();
      continue;
    }

    receivedFlag = false;
    int state = radio.readData(payload);
    radio.standby();
    flashLed();

    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("Packet received!"));
      Serial.print(F("Payload: "));
      Serial.println(payload);

      rssi = String(radio.getRSSI()) + "dBm";
      snr = String(radio.getSNR()) + "dB";
      drawMain();
    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      Serial.println(F("CRC error!"));
    } else {
      Serial.print(F("Receive failed, code "));
      Serial.println(state);
    }
  }

  Serial.println(F("Frequency sweep complete.\n"));
  delay(3000);  // Delay before starting next sweep
}


void drawMain()
{
    if (u8g2) {
        u8g2->clearBuffer();
        u8g2->drawRFrame(0, 0, 128, 64, 5);
        u8g2->setFont(u8g2_font_pxplusibmvga8_mr);
        u8g2->setCursor(15, 20);
        u8g2->print("RX:");
        u8g2->setCursor(15, 35);
        u8g2->print("SNR:");
        u8g2->setCursor(15, 50);
        u8g2->print("RSSI:");

        u8g2->setFont(u8g2_font_crox1h_tr);
        u8g2->setCursor( U8G2_HOR_ALIGN_RIGHT(payload.c_str()) - 21, 20 );
        u8g2->print(payload);
        u8g2->setCursor( U8G2_HOR_ALIGN_RIGHT(snr.c_str()) - 21, 35 );
        u8g2->print(snr);
        u8g2->setCursor( U8G2_HOR_ALIGN_RIGHT(rssi.c_str()) - 21, 50 );
        u8g2->print(rssi);
        u8g2->sendBuffer();
    }
}