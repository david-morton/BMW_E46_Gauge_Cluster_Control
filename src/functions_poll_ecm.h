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
void requestEcmDataOilTemp(mcp2515_can);
void requestEcmDataBatteryVoltage(mcp2515_can);
void requestEcmDataGasPedalPercentage(mcp2515_can);
void requestEcmDataAfRatioBank1(mcp2515_can);
void requestEcmDataAfRatioBank2(mcp2515_can);
void requestEcmDataAlphaPercentageBank1(mcp2515_can);
void requestEcmDataAlphaPercentageBank2(mcp2515_can);
void requestEcmDataAirIntakeTemp(mcp2515_can);

#endif
