#include "functions_write.h"
#include <mcp2515_can.h>        // Used for Seeed shields

/*****************************************************
 *
 * Function - Write the current temperature value to 
 * the CAN bus for cluster gauge display
 *
 ****************************************************/
unsigned char canPayloadTemp[8] = {0, 0, 0, 0, 0, 0, 0, 0};

void canWriteTemp(int currentEngineTempCelsius, mcp2515_can can){
    canPayloadTemp[1] = (currentEngineTempCelsius + 48.373) / 0.75;
    can.sendMsgBuf(0x329, 0, 8, canPayloadTemp);
}

/*****************************************************
 *
 * Function - Write the current RPM value to the CAN bus
 * for cluster gauge display
 *
 ****************************************************/
unsigned char canPayloadRpm[8] =  {0, 0, 0, 0, 0, 0, 0, 0};
int previousRpm;                                        // Will store the previous RPM value
int multipliedRpm;                                      // The RPM value to represent in CAN payload which the cluster is expecting
float rpmHexConversionMultipler = 6.55;                 // Default multiplier set to a sensible value for accuracy at lower RPM.
                                                        // This will be overriden via the formula based multiplier later on if used.

void canWriteRpm(int currentRpm, mcp2515_can can){
    if (currentRpm != 0 && abs(currentRpm - previousRpm) < 750){     // We see some odd values from time to time so lets filter them out
        rpmHexConversionMultipler = (-0.00005540102040816370 * currentRpm) + 6.70061224489796;
        multipliedRpm = currentRpm * rpmHexConversionMultipler;
        canPayloadRpm[2] = multipliedRpm;            //LSB
        canPayloadRpm[3] = (multipliedRpm >> 8);     //MSB
    }

    can.sendMsgBuf(0x316, 0, 8, canPayloadRpm);
    previousRpm = currentRpm;
}

/*****************************************************
 *
 * Function - Write the current speed value to the CAN bus
 *
 ****************************************************/
unsigned char canPayloadSpeed[8] = {0, 0, 0, 0, 0, 0, 0, 0};

void canWriteSpeed(int currentVehicleSpeed, mcp2515_can can){
    canPayloadSpeed[4] = currentVehicleSpeed;
    canPayloadSpeed[5] = currentVehicleSpeed >> 8;
    can.sendMsgBuf(0x280, 0, 8, canPayloadSpeed);
    can.sendMsgBuf(0x284, 0, 8, canPayloadSpeed);
}