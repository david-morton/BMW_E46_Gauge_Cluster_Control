#ifndef FUNCTIONS_READ_H
#define FUNCTIONS_READ_H

#include <Adafruit_MCP9808.h> // Used for temperature sensor
#include <Arduino.h>
#include <mcp2515_can.h> // Used for Seeed shields

/****************************************************
 *
 * Custom Data Types
 *
 ****************************************************/
struct nissanCanValues {
  // The below streamed all the time from ECM
  int engineTempCelsius = 0;
  // The below returned from sending queries to the ECM
  int oilTempCelcius = 0;
  float batteryVoltage = 0;
  int fuelTankTemp = 0;
  int airIntakeTemp = 0;
  int gasPedalPercentage = 0;
  float airFuelRatioBank1 = 0;
  float airFuelRatioBank2 = 0;
  int alphaPercentageBank1 = 0;
  int alphaPercentageBank2 = 0;
  int throttlePositionBank1 = 0;
  int throttlePositionBank2 = 0;
  float injectorDurationBank1 = 0;
  float injectorDurationBank2 = 0;
  int checkEngineLightState;
};

// Define stucture for holding last update timestamps for values. We will set values to 0 as needed
// to indicate stale data for a particular metric and to indicate it can not be trusted.
struct nissanCanUpdateTimes {
  unsigned long engineTempCelsius = 0;
  unsigned long oilTempCelcius = 0;
  unsigned long batteryVoltage = 0;
  unsigned long fuelTankTemp = 0;
  unsigned long intakeAirTemp = 0;
  unsigned long gasPedalPercentage = 0;
  unsigned long airFuelRatioBank1 = 0;
  unsigned long airFuelRatioBank2 = 0;
  unsigned long throttlePositionBank1 = 0;
  unsigned long throttlePositionBank2 = 0;
  unsigned long injectorDurationBank1 = 0;
  unsigned long injectorDurationBank2 = 0;
};

struct bmwCanValues {
  float vehicleSpeedFront;
  float vehicleSpeedRear;
  float vehicleSpeedRearVariation;
  unsigned long timestamp;
};

/****************************************************
 *
 * Function Prototypes
 *
 ****************************************************/
float readEngineElectronicsTemp(Adafruit_MCP9808);
nissanCanValues readNissanDataFromCan(mcp2515_can);
bmwCanValues readBmwDataFromCan(mcp2515_can);
float calculateAfRatioFromVoltage(float);

#endif
