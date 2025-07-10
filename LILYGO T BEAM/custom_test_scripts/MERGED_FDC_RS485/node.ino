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
uint8_t MEASUREMENT[] = { 0, 1, 2, 3 };
uint8_t CHANNEL[] = { 0, 1, 2, 3 };
uint8_t CAPDAC[] = { 0, 0, 0, 0 };
int capdac = 0;
int32_t rawCapacitance[4];
float capacitance[4];
float avgCapacitance[4];
float waterVol[4];
FDC1004 FDC;

// Main reading function of the FDC chip
void fdcRead(uint8_t* MEASUREMENT, uint8_t* CHANNEL, uint8_t* CAPDAC, int32_t* rawCapacitance) {
  for (int i = 0; i < 4; i++) {
    uint8_t measurement = MEASUREMENT[i];
    uint8_t channel = CHANNEL[i];
    uint8_t capdac = CAPDAC[i];

    FDC.configureMeasurementSingle(measurement, channel, capdac);
    FDC.triggerSingleMeasurement(measurement, FDC1004_100HZ);

    //wait for completion
    delay(15);
    uint16_t value[2];
    if (!FDC.readMeasurement(measurement, value)) {
      int16_t msb = (int16_t)value[0];
      /*int32_t*/ rawCapacitance[i] = ((int32_t)457) * ((int32_t)msb);  //in attofarads
      rawCapacitance[i] /= 1000;                                        //in femtofarads
      rawCapacitance[i] += ((int32_t)3028) * ((int32_t)capdac);
      capacitance[i] = (float)rawCapacitance[i] / 1000; //in picofarads

      if (msb > UPPER_BOUND)  // adjust capdac accordingly
      {
        if (CAPDAC[i] < FDC1004_CAPDAC_MAX)
          CAPDAC[i]++;
      } else if (msb < LOWER_BOUND) {
        if (CAPDAC[i] > 0)
          CAPDAC[i]--;
      }
    }
  }
}

// Averages 10 capacitance readings and converts it to water volume
void fdcReadAverage() {

  float average[] = { 0, 0, 0, 0 };

  for (int i = 0; i < 10; i++) {
    fdcRead(MEASUREMENT, CHANNEL, CAPDAC, rawCapacitance);
    average[0] += capacitance[0];
    average[1] += capacitance[1];
    average[2] += capacitance[2];
    average[3] += capacitance[3];
  }

  Serial.println("Average capacitance readings:");
  Serial.print("Channel 1: ");
  Serial.print(average[0] / 10);
  Serial.println(" pF");
  Serial.print("Channel 2: ");
  Serial.print(average[1] / 10);
  Serial.println(" pF");
  // Serial.print("Channel 3: ");
  // Serial.print(average[2] / 10);
  // Serial.println(" pF");
  // Serial.print("Channel 4: ");
  // Serial.print(average[3] / 10);
  // Serial.println(" pF");

  for (int i = 0; i < 4; i++) {
    avgCapacitance[i] = average[i] /= 10;
    // converts capacitance to water vol with equation derived from sensor experiments
    waterVol[i] = avgCapacitance[i] * 0.7775 - 11.325; // volume in microlitre
  }
}


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
  
  fdcRead(MEASUREMENT, CHANNEL, CAPDAC, rawCapacitance);
  fdcReadAverage();
  delay(2000);

}
