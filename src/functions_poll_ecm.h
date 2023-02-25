#ifndef FUNCTIONS_POLL_ECM_H
#define FUNCTIONS_POLL_ECM_H

#include <Arduino.h>
#include <mcp2515_can.h> // Used for Seeed shields

/****************************************************
 *
 * Function Prototypes
 *
 ****************************************************/
void initialiseEcmForQueries(mcp2515_can);

#endif
