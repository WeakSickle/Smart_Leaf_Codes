// Header file for soild sensor helper functions using the RS485 Protocol
#ifndef SOIL_SENSOR_H
#define SOIL_SENSOR_H

#include <Arduino.h>
#include <LoRaBoards.h>

class SoilSensor {
  private: 
    uint8_t DeviceAddress;
    uint32_t baud = 9600; // Default baud rate for RS485 communication
    int RS485_RX_PIN = 46;
    int RS485_TX_PIN = 39;
    int RE_DE_PIN = 45; 

    uint16_t Temp;
    uint16_t Moisture;
    uint16_t EC;
    uint16_t PH;
    // Helper functions for comms
    void setTransmit(bool tx);
    bool sendPacket(uint8_t *data, uint8_t len);
    uint16_t crc16(uint8_t *buf, uint8_t len);
    bool readResponse(uint8_t *buf, uint8_t len, uint16_t timeout_ms);
  public:
    SoilSensor(uint8_t address = 0x08) : DeviceAddress(address) {};
    bool findAddress();
    void begin();
    bool readSensor(uint8_t (&respone)[13]);
    uint16_t GetTemperature(uint8_t (&response)[13]);
    uint16_t GetMoisture(uint8_t (&response)[13]);
    uint16_t GetEC(uint8_t (&response)[13]);
    uint16_t GetPH(uint8_t (&response)[13]);
};

#endif