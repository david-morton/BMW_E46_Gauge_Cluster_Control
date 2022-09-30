#ifndef FUNCTIONS_READ_H
#define FUNCTIONS_READ_H

#include <Arduino.h>
#include <Adafruit_MCP9808.h>   // Used for temperature sensor
#include <mcp2515_can.h>        // Used for Seeed shields

// Function Definitions
float readEngineElectronicsTemp(Adafruit_MCP9808);
void readNissanDataFromCan(mcp2515_can);

#endif
