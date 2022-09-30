#ifndef FUNCTIONS_READ_H
#define FUNCTIONS_READ_H

#include <Arduino.h>
#include <Adafruit_MCP9808.h>   // Used for temperature sensor
#include <mcp2515_can.h>        // Used for Seeed shields

// Custom data types
struct nissanCanValues
{
    int engineTempCelsius;
    int checkEngineLightState;
};

// Function Definitions
float readEngineElectronicsTemp(Adafruit_MCP9808);
nissanCanValues readNissanDataFromCan(mcp2515_can);

#endif
