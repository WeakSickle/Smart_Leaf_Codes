# 1 "C:\\Users\\iv3n3\\AppData\\Local\\Temp\\tmpzl51lugs"
#include <Arduino.h>
# 1 "D:/Ivan/University - Engineering/700A/Smart_Leaf_Codes/LILYGO T BEAM/examples/GPS/UBlox_BasicNMEARead/UBlox_BasicNMEARead.ino"






#include "SparkFun_Ublox_Arduino_Library.h"
#include "LoRaBoards.h"

SFE_UBLOX_GPS myGPS;
void setup();
void loop();
#line 12 "D:/Ivan/University - Engineering/700A/Smart_Leaf_Codes/LILYGO T BEAM/examples/GPS/UBlox_BasicNMEARead/UBlox_BasicNMEARead.ino"
void setup()
{
    setupBoards();


    delay(1500);

    Serial.println("SparkFun Ublox Example");
    myGPS.enableDebugging();

    if (myGPS.begin(SerialGPS) == false) {
        Serial.println(F("Ublox GPS not detected at default I2C address. Please check wiring. Freezing."));
        while (1);
    }


    myGPS.setNMEAOutputPort(Serial);
}

void loop()
{
    myGPS.checkUblox();

    delay(250);
}