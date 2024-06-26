#include "functions_poll_ecm.h"
#include <mcp2515_can.h> // Used for Seeed shields

/*****************************************************
 *
 * Function - Get the ECM in a state where we can query parameters on it
 *
 ****************************************************/
void initialiseEcmForQueries(mcp2515_can CAN_NISSAN) {
  Serial.println("Sending request for diagnostic mode so we can query ECU ...");
  CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x02, 0x10, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00});
  delay(50);
  CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x02, 0x10, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00});
  delay(50);
  // CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x02, 0x21, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00});
  // delay(50);
  // CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x03, 0x22, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00});
  // delay(50);
  // CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x03, 0x22, 0x11, 0x40, 0x00, 0x00, 0x00, 0x00});
  // delay(50);
  // CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x03, 0x22, 0x11, 0x60, 0x00, 0x00, 0x00, 0x00});
  // delay(50);
  // CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x03, 0x22, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00});
  // delay(50);
  // CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x03, 0x22, 0x12, 0x20, 0x00, 0x00, 0x00, 0x00});
  // delay(50);
  // CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x03, 0x22, 0x12, 0x40, 0x00, 0x00, 0x00, 0x00});
  // delay(50);
  // CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x03, 0x22, 0x11, 0x80, 0x00, 0x00, 0x00, 0x00});
  // delay(50);
  // CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x03, 0x22, 0x13, 0x01, 0x00, 0x00, 0x00, 0x00});
  // delay(50);
  // CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x03, 0x22, 0x13, 0x02, 0x00, 0x00, 0x00, 0x00});
  // delay(50);
  // CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x03, 0x22, 0x13, 0x03, 0x00, 0x00, 0x00, 0x00});
  // delay(50);
  // CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x03, 0x22, 0x13, 0x04, 0x00, 0x00, 0x00, 0x00});
  // delay(50);
  // CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x03, 0x22, 0x13, 0x05, 0x00, 0x00, 0x00, 0x00});
  // delay(50);
  // CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x03, 0x22, 0x13, 0x08, 0x00, 0x00, 0x00, 0x00});
  // delay(50);
  // CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x03, 0x22, 0x13, 0x09, 0x00, 0x00, 0x00, 0x00});
  // delay(50);
  // CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x03, 0x22, 0x13, 0x0A, 0x00, 0x00, 0x00, 0x00});
  // delay(50);
  // CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x03, 0x22, 0x13, 0x0B, 0x00, 0x00, 0x00, 0x00});
  // delay(50);
  // CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x03, 0x22, 0x13, 0x0D, 0x00, 0x00, 0x00, 0x00});
  // delay(50);
  // CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x03, 0x22, 0x13, 0x07, 0x00, 0x00, 0x00, 0x00});
  // delay(50);
  // CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, new uint8_t[8]{0x02, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
  delay(1000);
}

/*****************************************************
 *
 * Functions - Request for queried metrics from ECM
 *
 ****************************************************/
// Oil temp
void requestEcmDataOilTemp(mcp2515_can CAN_NISSAN) {
  unsigned char canPayload[8] = {0x03, 0x22, 0x11, 0x1F, 0x00, 0x00, 0x00, 0x00};
  CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, canPayload);
}

// Battery voltage
void requestEcmDataBatteryVoltage(mcp2515_can CAN_NISSAN) {
  unsigned char canPayload[8] = {0x03, 0x22, 0x11, 0x03, 0x00, 0x00, 0x00, 0x00};
  CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, canPayload);
}

// Pedal position
void requestEcmDataGasPedalPercentage(mcp2515_can CAN_NISSAN) {
  unsigned char canPayload[8] = {0x03, 0x22, 0x12, 0x0D, 0x00, 0x00, 0x00, 0x00};
  CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, canPayload);
}

// AF Voltage bank 1
void requestEcmDataAfRatioBank1(mcp2515_can CAN_NISSAN) {
  unsigned char canPayload[8] = {0x03, 0x22, 0x12, 0x25, 0x00, 0x00, 0x00, 0x00};
  CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, canPayload);
}

// AF Voltage bank 2
void requestEcmDataAfRatioBank2(mcp2515_can CAN_NISSAN) {
  unsigned char canPayload[8] = {0x03, 0x22, 0x12, 0x26, 0x00, 0x00, 0x00, 0x00};
  CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, canPayload);
}

// Alpha percentage bank 1
void requestEcmDataAlphaPercentageBank1(mcp2515_can CAN_NISSAN) {
  unsigned char canPayload[8] = {0x03, 0x22, 0x11, 0x23, 0x00, 0x00, 0x00, 0x00};
  CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, canPayload);
}

// Alpha percentage bank 2
void requestEcmDataAlphaPercentageBank2(mcp2515_can CAN_NISSAN) {
  unsigned char canPayload[8] = {0x03, 0x22, 0x11, 0x24, 0x00, 0x00, 0x00, 0x00};
  CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, canPayload);
}

// Air intake temp
void requestEcmDataAirIntakeTemp(mcp2515_can CAN_NISSAN) {
  unsigned char canPayload[8] = {0x03, 0x22, 0x11, 0x06, 0x00, 0x00, 0x00, 0x00};
  CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, canPayload);
}

/*****************************************************
 *
 * Functions - Request fault data from ECM
 *
 ****************************************************/
void requestEcmDataFaults(mcp2515_can CAN_NISSAN) {
  // unsigned char canPayloadInitialise[8] = {0x02, 0x10, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00};
  // CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, canPayloadInitialise);
  unsigned char canPayloadRequest[8] = {0x03, 0x17, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00};
  CAN_NISSAN.sendMsgBuf(0x7DF, 0, 8, canPayloadRequest);
}
