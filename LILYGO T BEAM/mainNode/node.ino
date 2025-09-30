#include <Arduino.h>
#include <RadioLib.h>
#include "Protocentral_FDC1004_EDITTED.h"
#include "LoRaBoards.h"
#include "SparkFun_Ublox_Arduino_Library.h"
#include "SoilSensor.h"
#include "transmit_utils.h"
// Radio Parameters
#define CONFIG_RADIO_FREQ 923.20

#define CONFIG_RADIO_OUTPUT_POWER 22

#define CONFIG_RADIO_BW 125.0

// Constant params
#define NODE_ID 1

// Setup for different levels of funtion
#define LOW_POWER_CONFIG // Use our power saving functionality
//#define USE_DISPLAY // Use the oled display
// #define USE_SOIL // Use the soil sensor
// #define USE_FDC // Use the FDC1004 sensor
#define USE_SLEEP

const unsigned long TRANSMISSION_DURATION_MS = 30000;
const uint64_t secondsToSleep = 60;


// Setup for the FDC
#define UPPER_BOUND 0X4000  // max readout capacitance
#define LOWER_BOUND (-1 * UPPER_BOUND)

int waterVolumeOne;
int waterVolumeTwo;

// Defining the radio module and GPS
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
SFE_UBLOX_GPS GPS;

// Setup for the soils sensor
SoilSensor FourParam;
uint8_t resp[13];
TRANSMIT_DATA data; // Struct to hold the data to be transmitted after

// FDC setupt
FDC1004 FDC;

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

// Function for checking the RADIO setup error codes 
bool checkError(int errCode, const char* errMsg) {
  if (errCode != RADIOLIB_ERR_NONE) {
    Serial.println(errMsg);
    while (true);
    return false;  // Will never reach here but good style
  }
  return true;
}

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

void DISPLAY_STATE()
{
  u8g2->clearBuffer(); // Clear the display buffer
  String stateString =
      "State: " +
      String(currentState); // Create a string with the current state
  u8g2->drawStr(0, 20,
                stateString.c_str()); // Draw the state string on the display
  u8g2->sendBuffer();                 // Send the buffer to the display
}

void setup()
{
  data.ID = NODE_ID;
  setupBoards(); // Setup the Board (pretty sure its the pins)

  delay(1500);


#ifdef LOW_POWER_CONFIG   // Initial power saving setup
  btStop();               // Disable the bluetooth to save power
  WiFi.mode(WIFI_OFF);    // Disable the WiFi to save power
  setCpuFrequencyMhz(80); // Reduce the clock speed to 80MHz to save power
                          // esp_sleep_enable_ext0_wakeup((gpio_num_t)RADIO_DIO1_PIN, HIGH); // NOT SURE IF THIS WILL WORK
  Serial.print("WIFI MODE: ");
  Serial.println(WiFi.getMode());
  Serial.print("CPU Freq: ");
  Serial.println(getCpuFrequencyMhz());
#endif

  if (!u8g2)
  { // Ensure OLED SCREEN found
    Serial.println(
        "No find SH1106 display!Please check whether the connection is normal");
    while (1)
      ;
  }

  // Checks for setting up the LoRA transmitter
  // initialize radio with default settings
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

  checkError(radio.setFrequency(CONFIG_RADIO_FREQ), "Selected frequency is invalid for this module!");
  checkError(radio.setBandwidth(CONFIG_RADIO_BW), "Selected bandwidth is invalid for this module!");
  checkError(radio.setSpreadingFactor(12), "Selected spreading factor is invalid for this module!");
  checkError(radio.setCodingRate(6), "Selected coding rate is invalid for this module!");
  checkError(radio.setSyncWord(0xAB), "Unable to set sync word!");
  checkError(radio.setOutputPower(CONFIG_RADIO_OUTPUT_POWER), "Selected output power is invalid for this module!");

  ///////
  // radio.sleep(); // Put the radio to sleep to save power
  Serial.print(F("Passed Radio Setup ... "));
  ///////
                // Start the I2C bus
  GPS.begin(SerialGPS);           // Start the GPS module
  Serial.print(F("Passed GPS Setup ... "));
  #ifdef USE_SOIL
    FourParam.begin(); // Initialize the soil sensor
  #endif

  delay(1000);                    // Wait for the GPS to start up
  GPS.setI2COutput(COM_TYPE_UBX); // Set the GPS to output UBX messages for I2C so is less power hungry
  // GPS.saveConfiguration(); //Save the current settings to flash and BBR
  Serial.print(F("Passed GPS ... "));
// (probably un comment if works)
  u8g2->begin();
#ifdef USE_DISPLAY
  // Initialize the display
  u8g2->setFont(u8g2_font_ncenB08_tr); // Set a readable font
#else 
Serial.println("Not using display");
u8g2->setPowerSave(1); // Turn off the display if not used
#endif

#ifdef LOW_POWER_CONFIG
  pinMode(GPS_WAKEUP_PIN, OUTPUT);   // Set the GPS wakeup pin as output
  digitalWrite(GPS_WAKEUP_PIN, LOW); // Put the GPS to sleep
  radio.sleep();
#endif

}

void loop()
{
#ifdef USE_DISPLAY
  u8g2->clearBuffer();
  u8g2->drawStr(0, 20, "Current State:");
#endif

  switch (currentState)
  {
  case STANDBY:
  {


#ifdef USE_DISPLAY
    DISPLAY_STATE(); // Display the current state on the screen
#endif

#ifdef LOW_POWER_CONFIG
// radio.sleep();
//  radio.startReceiveDutyCycle(); // This makes the radio turn on an off periodically there is calculator for
// esp_deep_sleep_start();  // Put the ESP32 into deep sleep mode until a packet is received
#endif

    // delay(5000);
    currentState = GPS_ACQUISITION; // Move to the next state
    break;
  }
  case GPS_ACQUISITION: { // Acquire Satellite lock
    #ifdef LOW_POWER_CONFIG
        digitalWrite(GPS_WAKEUP_PIN, HIGH);  // Wake up GPS once at start
        delay(1000);  // Stabilization time
    #endif

    #ifdef USE_DISPLAY
        DISPLAY_STATE();
    #endif

    const unsigned long timeout = 10000;
    unsigned long startTime = millis();
    bool gpsLockAcquired = false;

    while (!gpsLockAcquired && (millis() - startTime < timeout)) {
        if (SerialGPS.available()) {
            uint8_t fixType = GPS.getFixType();
            Serial.print("Fix type: "); Serial.println(fixType);

            if (fixType == 3) {  // 3D fix
                gpsLockAcquired = true;
                currentState = GPS_LOCK;
                break;
            }
        }
        delay(500);
    }

    if (!gpsLockAcquired) {
      currentState = GPS_NO_LOCK;
      #ifdef LOW_POWER_CONFIG
        digitalWrite(GPS_WAKEUP_PIN, LOW);  // Put GPS to sleep
      #endif
    }
    break;
}

case GPS_LOCK: // Once lock is acquired do this step
{
  // GPS is locked read the data and store it for transmission
  GPS.getPVT(); // Get the PVT data from the GPS
  data.year = GPS.gpsYear;
  data.month = GPS.gpsMonth;
  data.day = GPS.gpsDay;
  data.hour = GPS.gpsHour;
  data.minute = GPS.gpsMinute;
  data.second = GPS.gpsSecond;

  data.latitude = GPS.latitude / 10000000.0;   // Convert to degrees
  data.longitude = GPS.longitude / 10000000.0; // Convert to degrees
  data.altitude = GPS.altitude / 1000.0;       // Convert to meters

  // Serial.print("Year: ");
  // Serial.print(data.year);
  // Serial.print(" Month: ");
  // Serial.print(data.month);
  // Serial.print(" Day: ");
  // Serial.print(data.day);
  // Serial.print(" Hour: ");
  // Serial.print(data.hour);
  // Serial.print(" Minute: ");
  // Serial.print(data.minute);
  // Serial.print(" Second: ");
  // Serial.print(data.second);
  // Serial.print(" Latitude: ");
  // Serial.print(data.latitude);
  // Serial.print(" Longitude: ");
  // Serial.print(data.longitude);
  // Serial.print(" Altitude: ");
  // Serial.println(data.altitude);

#ifdef USE_DISPLAY
  DISPLAY_STATE(); // Display the current state on the screen
#endif

#ifdef LOW_POWER_CONFIG
  digitalWrite(GPS_WAKEUP_PIN, LOW); // Put the GPS back to sleep
#endif
  currentState = SENSOR_DATA;        // Move to the next state
  break;
}
case GPS_NO_LOCK:
{
#ifdef USE_DISPLAY
  DISPLAY_STATE(); // Display the current state on the screen
#endif

  data.year = -1;
  data.month = -1;
  data.day = -1;
  data.hour = -1;
  data.minute = -1;
  data.second = -1;

  data.latitude = -1;
  data.longitude = -1;
  data.altitude = -1;

  Serial.println("No GPS Lock acquired!");
  currentState = SENSOR_DATA; // Move to the next state
  // delay(5000);
  break;
}
case SENSOR_DATA:
{
#ifdef USE_DISPLAY
  DISPLAY_STATE(); // Display the current state on the screen
#endif

  Serial.println("Sensor data processing ... ");

  // Moisture Sensors
  #ifdef USE_FDC
  FDC.fdcRead(); // Read the FDC data
  Serial.println("FDC Readings: ");
  float channelOne = FDC.fdcReadAverageOne(); // Average the FDC data then print it
  float channelTwo = FDC.fdcReadAverageTwo();
  Serial.print("Channel 1: ");
  Serial.println(channelOne);
  Serial.print("Channel 2: ");
  Serial.println(channelTwo);
  waterVolumeOne = FDC.convertCapacitanceToWaterVolume(channelOne, 1);
  waterVolumeTwo = FDC.convertCapacitanceToWaterVolume(channelTwo, 2 );
  Serial.print("Water Volume 1: ");
  Serial.println(waterVolumeOne);
  Serial.print("Water Volume 2: ");
  Serial.println(waterVolumeTwo);

  // delay(2000);
  #endif

  #ifdef USE_SOIL
    // Reads the soil sensor and save the data 
    FourParam.readSensor(resp);
    bool result = FourParam.readSensor(resp);
    if (result) {
      data.Temperature = FourParam.GetTemperature(resp);
      data.Moisture = FourParam.GetMoisture(resp);
      data.EC = FourParam.GetEC(resp);
      data.PH = FourParam.GetPH(resp);
      Serial.print("Temperature: ");
      Serial.println(FourParam.GetTemperature(resp));
      Serial.print("Moisture: ");
      Serial.println(FourParam.GetMoisture(resp));
      Serial.print("EC: ");
      Serial.println(FourParam.GetEC(resp));
      Serial.print("PH: ");
      Serial.println(FourParam.GetPH(resp));
    }
  #endif
  currentState = PMU_INFO; // Move to the next state
  break;
}
case PMU_INFO:
{ // This will have corresponding error codes passed in
  // each already if battery is not detected

#ifdef USE_DISPLAY
  DISPLAY_STATE(); // Display the current state on the screen
#endif

  Serial.println("PMU processing ... ");
  // Check the battery is being detected

  float percentage = PMU->getBatteryPercent(); // Apparently this is better after a
                                               // charge and discharge cycle so ill get
                                               // voltage aswell
  // Dunno how it will know though especially being turned on and off

  uint16_t BatV = PMU->getBattVoltage(); // in mV

  // Apparently it has a temperature sensor but the get temp is not
  // exposed in interface for some reason the function is there

  // PMU->enableTemperatureMeasure();
  // PMU->getTemperature();

  bool chargingStatus = PMU->isCharging();

  // Store the data into the struct for transmission
  data.batteryPercentage = percentage;
  data.batteryVoltage = BatV;
  data.isCharging = chargingStatus;

  currentState = TRANSMIT; // Move to the next state
  // delay(1000);
  break;
}
case TRANSMIT:
{
#ifdef USE_DISPLAY
  DISPLAY_STATE(); // Display the current state on the screen
#endif
  radio.standby(); // Wake the radio up and put in standby mode
  Serial.println("Transmitting data ... ");

String message = FormatMessage(data); // Format the data into a string for transmission

Serial.print("Message to send: ");
Serial.println(message);

  unsigned long startTime = millis();
  unsigned long endTime = startTime + TRANSMISSION_DURATION_MS;

  // WIll need to check if this is correct implementation
  Serial.println(transmittedFlag);
  // Loop and keep transmitting until the period is reached
  while (millis() < endTime) {
    if (transmittedFlag) {
      transmittedFlag = false; // Reset the flag for the next transmission

      // Check if the previous transmission was successful
      if (transmissionState == RADIOLIB_ERR_NONE) {
        Serial.println(F("Transmission finished successfully!"));
      } else {
        Serial.print(F("Transmission failed, code: "));
        Serial.println(transmissionState);
      }

      // Start the next transmission
      transmissionState = radio.startTransmit(message);
      Serial.print("MESSAGE SENT: ");
      Serial.println(message);
    }
  }
    
  currentState = STANDBY; // Move back to standby state
  radio.sleep();
  bool result = GPS.powerOff(secondsToSleep); // Power down the GPS to save power
  Serial.println("GPS powered down: " + String(result));
  sleepDevice(secondsToSleep); // Sleep for the defined time
  break;
}
}
}
