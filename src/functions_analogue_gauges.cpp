#include "functions_analogue_gauges.h"
#include "Arduino.h"
#include "globalHelpers.h"

/* ======================================================================
   FUNCTION: Calculate pressure sensor readings
   ====================================================================== */
float gaugeReadPressurePsi(int muxChannel) {
  // Sensor data shows the formula to be: psi = 36.25 * voltage - 18.125
  int sensorValue = getAveragedMuxAnalogueChannelReading(muxChannel, 5, 0);
  float voltage = sensorValue * 5.0 / 1023.0;
  float pressure = 36.25 * voltage - 18.125;
  if (pressure <= 0) {
    return 0;
  }
  return pressure;
}

/* ======================================================================
   FUNCTION: Calculate vacuum sensor readings
   ====================================================================== */
//  Measurement of sensor voltages vs vacuum gives the formula used in the below calculation pressure_psi = (voltage - 0.5707) / 0.0471
//  In our formula 0.5707 is the voltage reading at 0psi which we use as a default but set to the current measured value if we can during setup.
float gaugeReadVacuumPsi(int muxChannel, float atmospheric_voltage) {
  int sensorValue = getAveragedMuxAnalogueChannelReading(muxChannel, 5, 0);
  float voltage = sensorValue * (5.0 / 1023.0);
  float pressure_psi = (voltage - atmospheric_voltage) / 0.0471;
  return pressure_psi;
};

/* ======================================================================
   FUNCTION: Calculate temperature sensor readings
   ====================================================================== */
// Based on a simple voltage divider circuit using a 1500 ohm resistor due to resistive sensors being used.
// Sample readings were taken and the Steinhart-Hart coefficients were calculated as below for A, B and C via
// https://www.thinksrs.com/downloads/programs/therm%20calc/ntccalibrator/ntccalculator.html
const float R1 = 1500.0;
const float A = 0.9660089089e-3;
const float B = 2.385522641e-4;
const float C = 3.073243393e-7;

float gaugeReadTemperatureCelcius(int muxChannel) {
  int sensorReading = getAveragedMuxAnalogueChannelReading(muxChannel, 5, 0); // Get voltage from voltage divided sensor
  float sensorResistance = R1 * ((1023.0 / sensorReading) - 1.0);             // Convert voltage reading to resistance
  float tempKelvin = 1 / (A + B * log(sensorResistance) + C * log(sensorResistance) * log(sensorResistance) * log(sensorResistance));
  float temperatureC = tempKelvin - 273.15 + 10; // Measured readings vs OEM sensors (and known ambient)
                                                 // show a 10 degree variance
  return temperatureC;
};
