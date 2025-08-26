#include <Arduino.h>
#include <RadioLib.h>
#include "LoRaBoards.h"
#include "SparkFun_Ublox_Arduino_Library.h"
#include "Protocentral_FDC1004_EDITTED.h"
#include "SoilSensor.h"
// Radio Parameters
#define CONFIG_RADIO_FREQ 850.0

#define CONFIG_RADIO_OUTPUT_POWER 22

#define CONFIG_RADIO_BW 125.0

// uint8_t com[8] = {DeviceAddress,FunctionCode,0x00,0x00,0x00,NumberOfRegisters,0x44,0x0C};
SoilSensor FourParam;
uint8_t resp[13];
bool result = false;

// Constant params
#define NODE_ID 1

// Setup for different levels of funtion
#define LOW_POWER_CONFIG // Use our power saving functionality

#define USE_DISPLAY // Use the oled display

// Setup for the FDC
#define UPPER_BOUND 0X4000  // max readout capacitance
#define LOWER_BOUND (-1 * UPPER_BOUND)

// Defining the radio module and GPS
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
SFE_UBLOX_GPS GPS;

enum STATE
{
  STANDBY,
  GPS_ACQUISITION,
  GPS_LOCK,
  GPS_NO_LOCK,
  SENSOR_DATA,
  PMU_INFO,
  TRANSMIT
};

STATE currentState = STANDBY; // Default state is STANDBY (This will be low power mode most)

bool receivedFlag;
static volatile bool transmittedFlag = true; // Flag to indicate that a packet was received
static int transmissionState = RADIOLIB_ERR_NONE;
struct TRANSMIT_DATA
{
  int ID;
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  float latitude;
  float longitude;
  float altitude;
  bool isCharging;
  uint16_t batteryVoltage;
  int batteryPercentage;
};

// FDC 4 channel initialisation
// FDC1004 FDC;


TRANSMIT_DATA data; // Struct to hold the data to be transmitted after
                    // completeion of processing

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

void setup()
{
  //Wire.begin();
  GPS.begin(SerialGPS);
  delay(1000);
  GPS.setI2COutput(COM_TYPE_UBX); // Set the I2C output to UBX
  data.ID = NODE_ID;

  setupBoards(); // Setup the Board (pretty sure its the pins)

  Serial.begin(115200); // Start the serial communication

  FourParam.begin();
  delay(3000);
}

void loop()
{
  GPS.getPVT();

  Serial.print("Year: ");
  Serial.print(GPS.gpsYear);
  Serial.print(" Month: ");
  Serial.print(GPS.gpsMonth);
  Serial.print(" Day: ");
  Serial.print(GPS.gpsDay);
  Serial.print(" Hour: ");
  Serial.print(GPS.gpsHour);
  Serial.print(" Minute: ");
  Serial.print(GPS.gpsMinute);
  Serial.print(" Second: ");
  Serial.print(GPS.gpsSecond);
  Serial.print(" Latitude: ");
  Serial.print(GPS.latitude / 10000000.0);
  Serial.print(" Longitude: ");
  Serial.print(GPS.longitude / 10000000.0);
  Serial.print(" Altitude: ");
  Serial.println(GPS.altitude / 1000.0);

  result = FourParam.readSensor(resp);
    if (result) {
      Serial.print("Temperature: ");
      Serial.println(FourParam.GetTemperature(resp));
      Serial.print("Moisture: ");
      Serial.println(FourParam.GetMoisture(resp));
      Serial.print("EC: ");
      Serial.println(FourParam.GetEC(resp));
      Serial.print("PH: ");
      Serial.println(FourParam.GetPH(resp));
    }

  // FDC.fdcRead(); // Read the FDC data
  // FDC.fdcReadAverage(); // Average the FDC data then print it
  delay(2000);

}
