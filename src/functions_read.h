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
  int engineTempCelsius;
  int checkEngineLightState;
};

struct bmwCanValues {
  float vehicleSpeed;
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

#endif
