#include "functions_analogue_gauges.h"
#include "Arduino.h"

/****************************************************
 *
 * Function - Calculate pressure sensor readings
 *
 ****************************************************/
float gaugeReadPressurePsi(int sensorPin) {
  // Sensor data shows the formula to be: psi = 36.25 * voltage - 18.125
  int sensorValue = analogRead(sensorPin);
  float voltage = sensorValue * 5.0 / 1023.0;
  float pressure = 36.25 * voltage - 18.125;
  return pressure;
}

/****************************************************
 *
 * Function - Calculate vacuum sensor readings
 *
 ****************************************************/
float gaugeReadVacuumBar(int sensorPin) {
  return -0.05;
};

/****************************************************
 *
 * Function - Calculate temperature sensor readings
 * This is based on a voltage divider circuit to
 * work with the resistive sensors installed
 *
 ****************************************************/
float gaugeReadTemperatureCelcius(int sensorPin) {
  return 35.0;
};
