#ifndef FUNCTIONS_WRITE_H
#define FUNCTIONS_WRITE_H

#include <Arduino.h>
#include <mcp2515_can.h>        // Used for Seeed shields

// Function Definitions
void canWriteTemp(int, mcp2515_can);
void canWriteRpm(int, mcp2515_can);

#endif
