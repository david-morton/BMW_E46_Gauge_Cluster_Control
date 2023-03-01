#include "functions_analogue_gauges.h"
#include "Arduino.h"

// Define functions for reading gauge values
float gaugeReadPressurePsi(int sensorPin) {
  // Sensor data shows the formula to be: psi = 36.25 * voltage - 18.125
  int sensorValue = analogRead(sensorPin);
  float voltage = sensorValue * 5.0 / 1023.0;
  float pressure = 36.25 * voltage - 18.125;
  return pressure;
}

float gaugeReadVacuumBar(int sensorPin) { return -0.05; };

float gaugeReadTemperatureCelcius(int sensorPin) { return 35.0; };
