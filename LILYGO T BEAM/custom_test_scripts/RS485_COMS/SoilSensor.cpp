#include "SoilSensor.h"

// Initialise teh comm port for serial2
void SoilSensor::begin() {
    // Initialize the sensor, if needed
    pinMode(RE_DE_PIN, OUTPUT);
    digitalWrite(RE_DE_PIN, LOW);
    Serial2.begin(this->baud, SERIAL_8N1, this->RS485_RX_PIN, this->RS485_TX_PIN);
    delay(3000); // Allow time for sensor to stabilise on startup
    Serial.print("Soil Sensor initialized at address: ");
    Serial.println(this->DeviceAddress, HEX);
    
}

// Function that takes reference to response arrya of size 13 in main script and updates it wiht the sensor data (un converted)
bool SoilSensor::readSensor(uint8_t (&response)[13]) {
    while (Serial2.available()) {
        Serial2.read(); 
    }
    // Request data from sensor format: [DeviceAddress, FunctionCode, StartAddress (High and low), NumberOfRegisters, CRC_Low, CRC_High]
    uint8_t request[8] = {this->DeviceAddress, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00};
    uint16_t crc = crc16(request, 6);
    request[6] = crc & 0xFF;
    request[7] = crc >> 8;

    const int maxRetries = 5;
    int attempts = 0;
    bool SuccessfulMessage = false;

    while (!SuccessfulMessage && attempts < maxRetries) {
        sendPacket(request, 8);
        delay(150); // small delay after sending

        if (readResponse(response, 13, 1000)) {
            uint16_t response_crc = (response[12] << 8) | response[11];
            uint16_t calculated_crc = crc16(response, 11);

            if (response_crc == calculated_crc) {
                Serial.println("Response received and CRC valid.");

                Serial.println();

                SuccessfulMessage = true;
                return true; // Successfully read sensor data
            } else {
                Serial.println("CRC error in response.");
            }
        } else {
            Serial.println("No response or timeout.");
            return false;
        }
        attempts++;
    }

    if (!SuccessfulMessage) {
        Serial.println("Failed to get a valid response after retries.");
        return false;
    }
    return false;
}

uint16_t SoilSensor::GetTemperature(uint8_t (&response)[13]) {
    if (response[0] == this->DeviceAddress && response[1] == 0x03) {
        int16_t temp = (response[5] << 8) | response[6];
        return this->Temp = (temp / 10.0); // Return temperature in 0.1 degree Celsius
    }
    return this->Temp = 0; // Error or invalid response
}

uint16_t SoilSensor::GetMoisture(uint8_t (&response)[13]) {
    if (response[0] == this->DeviceAddress && response[1] == 0x03) {
        int16_t moisture = (response[3] << 8) | response[4];
        return this->Moisture = (moisture / 10.0); // Return moisture value (%)
    }
    return this->Moisture = 0; // Error or invalid response
}

uint16_t SoilSensor::GetEC(uint8_t (&response)[13]) {
    if (response[0] == this->DeviceAddress && response[1] == 0x03) {
        int16_t ec = (response[7] << 8) | response[8];
        return this->EC = ec; // Return EC value
    }
    return 0; // Error or invalid response
}

uint16_t SoilSensor::GetPH(uint8_t (&response)[13]) {
    if (response[0] == this->DeviceAddress && response[1] == 0x03) {
        int16_t ph = (response[9] << 8) | response[10];
        return this->PH = (ph / 100.0); // Return pH value
    }
    return this->PH = 0; // Error or invalid response
}

// Function to toggle the RS485 transmit/receive mode
void SoilSensor::setTransmit(bool tx) {
  digitalWrite(this->RE_DE_PIN, tx ? HIGH : LOW);
  delay(2);
}

bool SoilSensor::sendPacket(uint8_t *data, uint8_t len) {
  setTransmit(true);
  Serial2.write(data, len);
  Serial2.flush();
  setTransmit(false);
  return true;
}

// Attempt to read a response from the sensor
bool SoilSensor::readResponse(uint8_t *buf, uint8_t len, uint16_t timeout_ms) {
  uint32_t start = millis();
  uint8_t pos = 0;
  while ((millis() - start) < timeout_ms) {
    while (Serial2.available()) {
      buf[pos++] = Serial2.read();
      if (pos >= len) return true;
    }
  }
  return false;
}

// Check sum for RS485 Modbus RTU comms (from my understnanding this just check the integrity of the data)
uint16_t SoilSensor::crc16(uint8_t *buf, uint8_t len) {
  uint16_t crc = 0xFFFF;
  for (int i = 0; i < len; i++) {
    crc ^= buf[i];
    for (int j = 0; j < 8; j++) {
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
    }
  }
  return crc;
}