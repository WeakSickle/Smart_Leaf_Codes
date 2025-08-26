//////////////////////////////////////////////////////////////////////////////////////////
//
//    Arduino library for the FDC1004 capacitance sensor breakout board **EDITTED TO FIX RETURN ERROR**
//    Editor: Zaid Mustafa
//
//    Original Creditting below (editted line is commented)
//
//    Author: Ashwin Whitchurch
//    Copyright (c) 2018 ProtoCentral
//
//    Based on original code written by Benjamin Shaya
//
//    This software is licensed under the MIT License(http://opensource.org/licenses/MIT).
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
//   NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//   For information on how to use, visit https://github.com/protocentral/ProtoCentral_fdc1004_breakout
/////////////////////////////////////////////////////////////////////////////////////////

#include <Protocentral_FDC1004_EDITTED.h>
#include <LoRaBoards.h>

// Setup for the FDC
#define UPPER_BOUND 0X4000  // max readout capacitance
#define LOWER_BOUND (-1 * UPPER_BOUND)

// Constants and limits for FDC1004
#define FDC1004_UPPER_BOUND ((int16_t) 0x4000)
#define FDC1004_LOWER_BOUND (-1 * FDC1004_UPPER_BOUND)

uint8_t MEAS_CONFIG[] = {0x08, 0x09, 0x0A, 0x0B};
uint8_t MEAS_MSB[] = {0x00, 0x02, 0x04, 0x06};
uint8_t MEAS_LSB[] = {0x01, 0x03, 0x05, 0x07};
uint8_t SAMPLE_DELAY[] = {11,11,6,3};


uint8_t FDC1004::convertCapacitanceToWaterVolume(float capacitance, int sensorNumber) {
  // Conversion logic based on sensor calibration
  // This is a placeholder implementation
  if (sensorNumber == 1) {
    return (uint8_t)((capacitance - 17.226) * 1.155); // Example conversion for sensor 1
  } else if (sensorNumber == 2) {
    return (uint8_t)((capacitance - 21.551) * 2.930); // Example conversion for sensor 2
  } else if (sensorNumber == 3) {
    return (uint8_t)((capacitance - 14.132) * 4.534); // Example conversion for sensor 3
  } else if (sensorNumber == 4) {
    return (uint8_t)((capacitance - 20.528) * 0.68); // Example conversion for sensor 4
  } else if (sensorNumber == 5) {
    return (uint8_t)((capacitance - 15.673) * 0.056); // Example conversion for sensor 5
  } else  if (sensorNumber == 6) {
    return (uint8_t)((capacitance - 19.35) * 0.6); // Example conversion for sensor 6
  } else {
    return 0; // Invalid sensor number
  }
}

// Main reading function of the FDC chip
void FDC1004::fdcRead() {
  for (int i = 0; i < 4; i++) {
    uint8_t measurement = MEASUREMENT[i];
    uint8_t channel = CHANNEL[i];
    uint8_t capdac = CAPDAC[i];

    configureMeasurementSingle(measurement, channel, capdac);
    triggerSingleMeasurement(measurement, FDC1004_100HZ);
    //wait for completion
    delay(15);
    uint16_t value[2];
    if (!readMeasurement(measurement, value)) {
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
    } else {
      Serial.println("Measurement failed");
    }
  }
}

// Averages 10 capacitance readings and converts it to water volume
float FDC1004::fdcReadAverageOne() {

  float average[] = { 0, 0, 0, 0 };

  for (int i = 0; i < 10; i++) {
    fdcRead();
    average[0] += capacitance[0];
  }
  return average[0] / 10;

}

float FDC1004::fdcReadAverageTwo() {

  float average[] = { 0, 0, 0, 0 };

  for (int i = 0; i < 10; i++) {
    fdcRead();
    average[1] += capacitance[1];
  }
  return average[1] /10;

}

FDC1004::FDC1004(uint16_t rate)
{
  this->_addr = 0b1010000;
  this->_rate = rate;
}

void FDC1004::write16(uint8_t reg, uint16_t data)
{
  Wire.beginTransmission(_addr);
  Wire.write(reg); //send address
  Wire.write( (uint8_t) (data >> 8));
  Wire.write( (uint8_t) data);
  Wire.endTransmission();
}

uint16_t FDC1004::read16(uint8_t reg)
{
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    Wire.endTransmission(false); // Send a restart, not a stop

    Wire.requestFrom(_addr, (uint8_t)2);
    if (Wire.available() < 2) {
        return 0; // or handle error
    }
    uint16_t value = Wire.read();
    value <<= 8;
    value |= Wire.read();
    return value;
}

//configure a measurement
uint8_t FDC1004::configureMeasurementSingle(uint8_t measurement, uint8_t channel, uint8_t capdac)
{
    //Verify data
    if (!FDC1004_IS_MEAS(measurement) || !FDC1004_IS_CHANNEL(channel) || capdac > FDC1004_CAPDAC_MAX) {
        Serial.println("bad configuration");
        return 1;
    }

    //build 16 bit configuration
    uint16_t configuration_data;
    configuration_data = ((uint16_t)channel) << 13; //CHA
    configuration_data |=  ((uint16_t)0x04) << 10; //CHB disable / CAPDAC enable
    configuration_data |= ((uint16_t)capdac) << 5; //CAPDAC value
    write16(MEAS_CONFIG[measurement], configuration_data);
    return 0;
}

uint8_t FDC1004::triggerSingleMeasurement(uint8_t measurement, uint8_t rate)
{
  //verify data
    if (!FDC1004_IS_MEAS(measurement) || !FDC1004_IS_RATE(rate)) {
        Serial.println("bad trigger request");
        return 1;
    }
    uint16_t trigger_data;
    trigger_data = ((uint16_t)rate) << 10; // sample rate
    trigger_data |= 0 << 8; //repeat disabled
    trigger_data |= (1 << (7-measurement)); // 0 > bit 7, 1 > bit 6, etc
    write16(FDC_REGISTER, trigger_data);
    return 0; // **EDITTED LINE**
}

/**
 * Check if measurement is done, and read the measurement into value if so.
  * value should be at least 4 bytes long (24 bit measurement)
 */
uint8_t FDC1004::readMeasurement(uint8_t measurement, uint16_t * value)
{
    if (!FDC1004_IS_MEAS(measurement)) {
        Serial.println("bad read request");
        return 1;
    }
    //check if measurement is complete
    uint16_t fdc_register = read16(FDC_REGISTER);
    if (! (fdc_register & ( 1 << (3-measurement)))) {
        Serial.println("measurement not completed");
        return 2;
    }
  //read the value
  uint16_t msb = read16(MEAS_MSB[measurement]);
  uint16_t lsb = read16(MEAS_LSB[measurement]);
  value[0] = msb;
  value[1] = lsb;
  //store the capdac value

  return 0;

}

/**
 * take a measurement, uses the measurement register equal to the channel number
 */
uint8_t FDC1004::measureChannel(uint8_t channel, uint8_t capdac, uint16_t * value)
{
  uint8_t measurement = channel; //4 measurement configs, 4 channels, seems fair
  if (configureMeasurementSingle(measurement, channel, capdac)) return 1;
  if (triggerSingleMeasurement(measurement, this->_rate)) return 1;
  delay(SAMPLE_DELAY[this->_rate]);
  return readMeasurement(measurement, value);
}

/**
 *  function to get the capacitance from a channel.
  */
int32_t FDC1004::getCapacitance(uint8_t channel)
{
    fdc1004_measurement_t value;
    uint8_t result = getRawCapacitance(channel, &value);
    if (result) return 0x80000000;

    int32_t capacitance = ((int32_t)ATTOFARADS_UPPER_WORD) * ((int32_t)value.value); //attofarads
    capacitance /= 1000; //femtofarads
    capacitance += ((int32_t)FEMTOFARADS_CAPDAC) * ((int32_t)value.capdac);
    return capacitance;
}
