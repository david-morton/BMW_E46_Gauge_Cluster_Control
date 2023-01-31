#include "functions_write.h"
#include <mcp2515_can.h> // Used for Seeed shields

/*****************************************************
 *
 * Function - Write the current temperature value to the CAN bus for cluster gauge display
 *
 ****************************************************/
unsigned char canPayloadTemp[8] = {0, 0, 0, 0, 0, 0, 0, 0};

void canWriteTemp(int currentEngineTempCelsius, mcp2515_can can) {
  canPayloadTemp[1] = (currentEngineTempCelsius + 48.373) / 0.75;
  can.sendMsgBuf(0x329, 0, 8, canPayloadTemp);
}

/*****************************************************
 *
 * Function - Write the current RPM value to the CAN bus for cluster gauge display
 *
 ****************************************************/
unsigned char canPayloadRpm[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int previousRpm;                        // Will store the previous RPM value
int multipliedRpm;                      // The RPM value to represent in CAN payload which the cluster is expecting
float rpmHexConversionMultipler = 6.55; // Default multiplier set to a sensible value for accuracy at lower
                                        // RPM. This will be overriden via the formula based multiplier later
                                        // on if used.

void canWriteRpm(int currentRpm, mcp2515_can can) {
  if (currentRpm != 0 && abs(currentRpm - previousRpm) < 750) { // We see some odd values from time
                                                                // to time so lets filter them out
    rpmHexConversionMultipler = (-0.00005540102040816370 * currentRpm) + 6.70061224489796;
    multipliedRpm = currentRpm * rpmHexConversionMultipler;
    canPayloadRpm[2] = multipliedRpm;        // LSB
    canPayloadRpm[3] = (multipliedRpm >> 8); // MSB
  }

  can.sendMsgBuf(0x316, 0, 8, canPayloadRpm);
  previousRpm = currentRpm;
}

/*****************************************************
 *
 * Function - Write the current speed value to the CAN bus for ECU consumption
 *
 ****************************************************/
unsigned char canPayloadSpeed[8] = {0, 0, 0, 0, 0, 0, 0, 0};

void canWriteSpeed(int currentVehicleSpeed, mcp2515_can can) {
  float speedValue = (0.3903 * currentVehicleSpeed) + 0.5144;
  canPayloadSpeed[4] = speedValue;
  // canPayloadSpeed[5] = speedValue >> 8;
  can.sendMsgBuf(0x284, 0, 8, canPayloadSpeed);
}

/*****************************************************
 *
 * Function - Write the current clutch status to the CAN bus for ECU consumption
 *
 ****************************************************/
unsigned char canPayloadClutchStatus[2] = {255, 255};
unsigned char canPayloadClutchStatusBig[8] = {255, 255, 255, 255, 255, 255, 255, 255};

void canWriteClutchStatus(int currentClutchStatus, mcp2515_can can) {
  // can.sendMsgBuf(0x216, 0, 2, canPayloadClutchStatus);
  can.sendMsgBuf(0x35d, 0, 8, canPayloadClutchStatusBig);

  // Debug to look at the bits which are set on a particular byte
  // SERIAL_PORT_MONITOR.println();
  // for (int i = 7; i >= 0; i--)
  // {
  //     bool b = bitRead(canPayloadClutchStatus[1], i);
  //     SERIAL_PORT_MONITOR.print(b);
  // }
}
