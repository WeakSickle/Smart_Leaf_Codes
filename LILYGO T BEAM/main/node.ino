#include <Arduino.h>
#include <RadioLib.h>

#include "LoRaBoards.h"
#include "SparkFun_Ublox_Arduino_Library.h"

// Radio Parameters
#define CONFIG_RADIO_FREQ 850.0

#define CONFIG_RADIO_OUTPUT_POWER 22

#define CONFIG_RADIO_BW 125.0


// Constant params 
#define NODE_ID 1

// Setup for different levels of funtion 
#define LOW_POWER_CONFIG // Use our power saving functionality

#define USE_DISPLAY // Use the oled display 

 
// Defining the radio module and GPS
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
SFE_UBLOX_GPS GPS;

enum STATE {
  STANDBY,
  GPS_ACQUISTION,
  GPS_LOCK,
  GPS_NO_LOCK,
  SENSOR_DATA,
  PMU_INFO,
  TRANSMIT
};

STATE currentState = STANDBY;  // Default state is STANDBY (This will be low power mode most)

bool receivedFlag;
static volatile bool transmittedFlag = false;  // Flag to indicate that a packet was received
static int transmissionState = RADIOLIB_ERR_NONE;
struct TRANSMIT_DATA {
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

TRANSMIT_DATA data;  // Struct to hold the data to be transmitted after
                     // completeion of processing

void setReceiveFlag(void) {
  // we got a packet, set the flag
  receivedFlag = true;
}

void setTransmitFlag(void) {
  // we got a packet, set the flag
  transmittedFlag = true;
}

void DISPLAY_STATE() {
  u8g2->clearBuffer();  // Clear the display buffer
  String stateString =
      "State: " +
      String(currentState);  // Create a string with the current state
  u8g2->drawStr(0, 20,
                stateString.c_str());  // Draw the state string on the display
  u8g2->sendBuffer();                  // Send the buffer to the display
}

void setup() {
  data.ID = NODE_ID;

  setupBoards();  // Setup the Board (pretty sure its the pins)

  delay(1500);

  #ifdef LOW_POWER_CONFIG
    btStop();             // Disable the bluetooth to save power
    WiFi.mode(WIFI_OFF);  // Disable the WiFi to save power
   // setCpuFrequencyMhz(80); // Reduce the clock speed to 80MHz to save power
   // esp_sleep_enable_ext0_wakeup((gpio_num_t)RADIO_DIO1_PIN, HIGH); // NOT SURE IF THIS WILL WORK
    Serial.print("WIFI MODE: ");
    Serial.println(WiFi.getMode());
  #endif

  if (!u8g2) {  // Ensure OLED SCREEN found
    Serial.println(
        "No find SH1106 display!Please check whether the connection is normal");
    while (1);
  }

  // Checks for setting up the LoRA transmitter
  // initialize radio with default settings
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

  // set the function that will be called
  // when packet transmission is finished
  
  radio.setPacketSentAction(setTransmitFlag);
  
  // radio.setPacketReceivedAction(setReceiveFlag);
  /*
   *   Sets carrier frequency.
   *   SX1268/SX1262 : Allowed values are in range from 150.0 to 960.0 MHz.
   * * * */
  if (radio.setFrequency(CONFIG_RADIO_FREQ) == RADIOLIB_ERR_INVALID_FREQUENCY) {
    Serial.println(F("Selected frequency is invalid for this module!"));
    while (true);
  }

  /*
   *   Sets LoRa link bandwidth.
   *   SX1268/SX1262 : Allowed values
   * are 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0 and 500.0 kHz.
   * * * */
  if (radio.setBandwidth(CONFIG_RADIO_BW) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
    Serial.println(F("Selected bandwidth is invalid for this module!"));
    while (true);
  }

  /*
   * Sets LoRa link spreading factor.
   * SX1262        :  Allowed values range from 5 to 12.
   * * * */
  if (radio.setSpreadingFactor(12) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
    Serial.println(F("Selected spreading factor is invalid for this module!"));
    while (true);
  }

  /*
   * Sets LoRa coding rate denominator.
   * SX1278/SX1276/SX1268/SX1262 : Allowed values range from 5 to 8. Only
   * available in LoRa mode.
   * * * */
  if (radio.setCodingRate(6) == RADIOLIB_ERR_INVALID_CODING_RATE) {
    Serial.println(F("Selected coding rate is invalid for this module!"));
    while (true);
  }

  /*
   * Sets LoRa sync word.
   * SX1278/SX1276/SX1268/SX1262/SX1280 : Sets LoRa sync word. Only available in
   * LoRa mode.
   * * */
  if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE) {
    Serial.println(F("Unable to set sync word!"));
    while (true);
  }
  
  /*
   * Sets transmission output power.
   * SX1262 :  Allowed values are in range from -9 to 22 dBm. This method is
   * virtual to allow override from the SX1261 class.
   * * * */
  if (radio.setOutputPower(CONFIG_RADIO_OUTPUT_POWER) ==
      RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
    Serial.println(F("Selected output power is invalid for this module!"));
    while (true);
  }

  Serial.print(F("Passed Radio Setpupwue ... "));

  Wire.begin(); // Start the I2C bus
  GPS.begin();  // Start the GPS module
  delay(1000); // Wait for the GPS to start up
  GPS.setI2COutput(COM_TYPE_UBX);  // Set the GPS to output UBX messages for I2C
                                   // so is less power hungry
  //GPS.saveConfiguration(); //Save the current settings to flash and BBR
  Serial.print(F("Passed GPS ... "));
  // (probably un comment if works)
  #ifdef USE_DISPLAY
    u8g2->begin();                        // Initialize the display
    u8g2->setFont(u8g2_font_ncenB08_tr);  // Set a readable font
  #endif


}

void loop() {
  #ifdef USE_DISPLAY
  u8g2->clearBuffer();
  u8g2->drawStr(0, 20, "Current State:");
  #endif

  switch (currentState) {
    case STANDBY: {
      radio.startReceive();  // Start listening for a packet to come (ie command
                            //  from master)
      
      #ifdef USE_DISPLAY
        DISPLAY_STATE();  // Display the current state on the screen
      #endif

      #ifdef LOW_POWER_CONFIG
      //  radio.startReceiveDutyCycle(); // This makes the radio turn on an off periodically there is calculator for 
        //esp_deep_sleep_start();  // Put the ESP32 into deep sleep mode until a packet is received 
      #endif

      delay(5000);
      currentState = GPS_ACQUISTION;  // Move to the next state
      break;
    }
    case GPS_ACQUISTION: {
      #ifdef LOW_POWER_CONFIG  // Will be using pin to control and set it to power
                         // save mode
        digitalWrite(GPS_WAKEUP_PIN, HIGH);  // Wake up the GPS module
        delay(1000);                         // Wait for the GPS to wake up
        GPS.powerSaveMode();                 // Put the GPS in power save mode
      #endif

      #ifdef USE_DISPLAY
        DISPLAY_STATE();  // Display the current state on the screen
      #endif

      // Start a timeout for GPS lock (if it is waking up but if continuous
      // should skip)
      const unsigned long timeout = 30000;  // 40 seconds
      unsigned long startTime = millis();
      while (millis() - startTime < timeout) {
        uint8_t fixType = GPS.getFixType();

        Serial.print("Fix type: ");
        Serial.println(fixType);

        if (fixType == 3) {
          Serial.println("GPS Lock acquired!");
          
          currentState = GPS_LOCK;  // Move to the next state
          break;
        }

        delay(500);  // Wait a bit before checking again
      }
      digitalWrite(GPS_WAKEUP_PIN, LOW);  // Put the GPS back to sleep
      currentState = GPS_NO_LOCK;  // Move to the next state
      break;
    }
    case GPS_LOCK: {
      // GPS is locked read the data and store it for transmission
      GPS.getPVT();  // Get the PVT data from the GPS
      data.year = GPS.gpsYear;
      data.month = GPS.gpsMonth;
      data.day = GPS.gpsDay;
      data.hour = GPS.gpsHour;
      data.minute = GPS.gpsMinute;
      data.second = GPS.gpsSecond;

      data.latitude = GPS.latitude / 10000000.0;  // Convert to degrees
      data.longitude = GPS.longitude / 10000000.0;  // Convert to degrees
      data.altitude = GPS.altitude / 1000.0;  // Convert to meters

      Serial.print("Year: ");
      Serial.print(data.year);
      Serial.print(" Month: ");
      Serial.print(data.month);
      Serial.print(" Day: ");
      Serial.print(data.day);
      Serial.print(" Hour: ");
      Serial.print(data.hour);
      Serial.print(" Minute: ");
      Serial.print(data.minute);
      Serial.print(" Second: ");
      Serial.print(data.second);
      Serial.print(" Latitude: ");
      Serial.print(data.latitude);
      Serial.print(" Longitude: ");
      Serial.print(data.longitude);
      Serial.print(" Altitude: ");
      Serial.println(data.altitude);

    #ifdef USE_DISPLAY
      DISPLAY_STATE();  // Display the current state on the screen
    #endif
      digitalWrite(GPS_WAKEUP_PIN, LOW);  // Put the GPS back to sleep
      currentState = SENSOR_DATA;  // Move to the next state
      break;
    }
    case GPS_NO_LOCK: {
    #ifdef USE_DISPLAY
      DISPLAY_STATE();  // Display the current state on the screen
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

      Serial.print("No GPS Lock acquired!");
      currentState = SENSOR_DATA;  // Move to the next state
      delay(5000);
      break;
    }
    case SENSOR_DATA: {
      #ifdef USE_DISPLAY
      DISPLAY_STATE();  // Display the current state on the screen
      #endif

      Serial.println("Sensor data processing ... ");
      delay(5000);
      currentState = PMU_INFO;  // Move to the next state
      break;
    }
    case PMU_INFO: {  // This will have corresponding error codes passed in
                      // eaach already if battery is not detected

      #ifdef USE_DISPLAY
        DISPLAY_STATE();  // Display the current state on the screen
      #endif

      Serial.println("PMU processing ... ");
      // Check the battery is being detected

      float percentage = PMU->getBatteryPercent();  // Apparently this is better after a
                                     // charge and discharge cycle so ill get
                                     // voltage aswell
      // Dunno how it will know though especially being turned on and off

      uint16_t BatV = PMU->getBattVoltage();  // in mV

      // Apparently it has a temperature sensor but the get temp is not
      // exposed in interface for some reason the function is there

      // PMU->enableTemperatureMeasure();
      // PMU->getTemperature();

      bool chargingStatus = PMU->isCharging();

      // Store the data into the struct for transmission
      data.batteryPercentage = percentage;
      data.batteryVoltage = BatV;
      data.isCharging = chargingStatus;

      currentState = TRANSMIT;  // Move to the next state
      delay(1000);
      break;
    }
    case TRANSMIT: {
          #ifdef USE_DISPLAY
            DISPLAY_STATE();  // Display the current state on the screen
          #endif

      Serial.println("Transmitting data ... ");
      // Read each of the parts from the struct adn put it into a string with
      // commas in between
      //String Time = String(data.year) + "," + String(data.month) + "," +
                    String(data.day) + "," + String(data.hour) + "," +
                    String(data.minute) + "," + String(data.second);
      String Position = String(data.latitude) + "," + String(data.longitude) +
                        "," + String(data.altitude);
      String Battery = String(data.batteryVoltage) + "," +
                       String(data.batteryPercentage) + "," +
                       String(data.isCharging);
      String message = String(data.ID) + "," + Position + "," + Battery;  // Combine the two strings into one message

      // WIll need to check if this is correct implementation
      if (transmittedFlag) {
        transmittedFlag = false;  // Reset the flag

        if (transmissionState == RADIOLIB_ERR_NONE) {
          // packet was successfully sent
          Serial.println(F("transmission finished!"));
          // NOTE: when using interrupt-driven transmit method,
          //       it is not possible to automatically measure
          //       transmission data rate using getDataRate()
          } else {
          Serial.print(F("failed, code "));
          Serial.println(transmissionState);
          }



        transmissionState = radio.startTransmit(message);  // Transmit the message, may need a check to ensure the message is finished
        // Transmitt
        Serial.print("MESSAGE SENT: ");
        Serial.print(message);
      }
      currentState = STANDBY;  // Move back to standby state
      delay(5000);
      break;
    }
  }
}
