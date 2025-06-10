#include "LoRaBoards.h"

// From my reading of code it seems that serial1 is gps so teh exposed tx adn rx pins are serial 2
# define RS485_RX_PIN 19
# define RS485_TX_PIN 20
# define RS485_BAUD_RATE 9600
uint8_t com[8] = {0x01,0x03,0x00,0x00,0x00,0x07,0x44,0x0C};

void setup() {
    setupBoards();  // Setup the Board (pretty sure its the pins)
    Serial.begin(115200);
    Serial2.begin(RS485_BAUD_RATE, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
}

void loop() {
  // For example, you could call setupBoards(), beginSDCard(), or other functions as needed.
}

void readSensor(void) {

}