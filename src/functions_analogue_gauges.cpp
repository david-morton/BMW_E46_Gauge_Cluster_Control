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
  if (pressure <= 0) {
    return 0;
  }
  return pressure;
}

/****************************************************
 *
 * Function - Calculate vacuum sensor readings
 *
 ****************************************************/
float gaugeReadVacuumBar(int sensorPin) {
  int sensorValue = analogRead(sensorPin);
  float voltage = sensorValue * (5.0 / 1023.0);
  float pressureBar = voltage - 1.00;
  // SERIAL_PORT_MONITOR.print("Sensor value: ");
  // SERIAL_PORT_MONITOR.println(sensorValue);
  // SERIAL_PORT_MONITOR.print("Vac voltage: ");
  // SERIAL_PORT_MONITOR.println(voltage);
  // SERIAL_PORT_MONITOR.print("Crank case vacuum: ");
  // SERIAL_PORT_MONITOR.println(pressureBar);
  return pressureBar;
};

/****************************************************
 *
 * Function - Calculate temperature sensor readings based on a simple voltage divider circuit using a 1500 ohm resistor
 * due to resistive sensors being used. Sample readings were taken and the Steinhart-Hart coefficients were calculated
 * as below for A, B and C via https://www.thinksrs.com/downloads/programs/therm%20calc/ntccalibrator/ntccalculator.html
 * 
 ****************************************************/
const float R1 = 1500.0;
const float A = 0.9660089089e-3;
const float B = 2.385522641e-4;
const float C = 3.073243393e-7;

float gaugeReadTemperatureCelcius(int sensorPin) {
  int sensorReading = analogRead(sensorPin);                        // Get voltage from voltage divided sensor
  float sensorResistance = R1 * ((1023.0 / sensorReading) - 1.0);   // Convert voltage reading to resistance
  float tempKelvin = 1 / (A + B*log(sensorResistance) + C*log(sensorResistance)*log(sensorResistance)*log(sensorResistance));
  float temperatureC = tempKelvin - 273.15;
  return temperatureC;
};
