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
float rpmHexConversionMultipler = 5.6; // Default multiplier set to a sensible value for accuracy at lower
                                        // RPM. This will be overriden via the formula based multiplier later
                                        // on if used. A value of 6.55 is used for OEM gauge and 5.6 for custom.

const int numPoints = 8;
int measuredRpmValues[numPoints] = {1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000};
float measuredHexConversionMultiplerValues[numPoints] = {5.9, 5.4, 5.2, 5.15, 5.1, 5.05, 5.0, 5.0};

void canWriteRpm(int currentRpm, mcp2515_can can) {
  if (currentRpm != 0 && abs(currentRpm - previousRpm) < 750) { // We see some odd values from time to time so lets filter them out

    // Calculate the rpmHexConversionMultipler based on linear interpolation of measured data points
    if (currentRpm <= 1200) {
        rpmHexConversionMultipler = 5.9;
    } else if (currentRpm >= 8000) {
        rpmHexConversionMultipler = 5.0;
    } else
    {
      // Find the two nearest RPM values in the table
      int i;
      for (i = 0; i < numPoints - 1; ++i) {
          if (measuredRpmValues[i + 1] >= currentRpm) {
              break;
          }
      }
      // Perform linear interpolation
      float x0 = measuredRpmValues[i];
      float x1 = measuredRpmValues[i + 1];
      float y0 = measuredHexConversionMultiplerValues[i];
      float y1 = measuredHexConversionMultiplerValues[i + 1];

      rpmHexConversionMultipler = y0 + (y1 - y0) * (currentRpm - x0) / (x1 - x0);
    }

    // Write the calculated value
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
  // can.sendMsgBuf(0x284, 0, 8, canPayloadSpeed);   // ID used for 2009 JDM Nissan Skyline 370GT ECU
  can.sendMsgBuf(0x280, 0, 8, canPayloadSpeed);      // ID used for 2011 USDM Nissan 370Z ECU
}

/*****************************************************
 *
 * Function - Write a diagnostic session keepalive
 *
 ****************************************************/
unsigned char canPayloadKeepalive[8] = {0x03, 0x22, 0x12, 0x01, 0x00, 0x00, 0x00, 0x00};

void canWriteDiagnosticKeepalive(mcp2515_can can) {
  can.sendMsgBuf(0x7DF, 0, 8, canPayloadKeepalive);
  // Serial.println("Sending keepalive");
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
  // Serial.println();
  // for (int i = 7; i >= 0; i--)
  // {
  //     bool b = bitRead(canPayloadClutchStatus[1], i);
  //     Serial.print(b);
  // }
}
