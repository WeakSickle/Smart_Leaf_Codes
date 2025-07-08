#include "LoRaBoards.h"
#include "SoilSensor.h"

// uint8_t com[8] = {DeviceAddress,FunctionCode,0x00,0x00,0x00,NumberOfRegisters,0x44,0x0C};
SoilSensor FourParam;
uint8_t resp[13];

void setup() {
    setupBoards();  // Setup the Board (pretty sure its the pins)
    Serial.begin(115200);
    FourParam.begin();
    // Serial2.begin(RS485_BAUD_RATE, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
}

void loop() {
  // For example, you could call setupBoards(), beginSDCard(), or other functions as needed.
    FourParam.readSensor(resp);
    Serial.print("Temperature: ");
    Serial.println(FourParam.GetTemperature(resp));
    Serial.print("Moisture: ");
    Serial.println(FourParam.GetMoisture(resp));
    Serial.print("EC: ");
    Serial.println(FourParam.GetEC(resp));
    Serial.print("PH: ");
    Serial.println(FourParam.GetPH(resp));

  
  delay(3000);
}
