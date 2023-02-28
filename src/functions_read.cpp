#include "functions_read.h"
#include <Adafruit_MCP9808.h> // Used for temperature sensor
#include <mcp2515_can.h>      // Used for Seeed shields

/*****************************************************
 *
 * Function - Get the current engine bay electronics temp
 *
 ****************************************************/
float readEngineElectronicsTemp(Adafruit_MCP9808 temp) { return temp.readTempC(); }

/*****************************************************
 *
 * Function - Read latest values from Nissan CAN
 *
 ****************************************************/
// Define min and max gas pedal voltages as measued from UpRev
float gasPedalMinVoltage = 0.65;
float gasPedalMaxVoltage = 4.85;
float gasPedalVoltageRange = gasPedalMaxVoltage - gasPedalMinVoltage;

// Define our custom struct for holding the values
nissanCanValues latestNissanCanValues; 
int checkEngineLightState;

nissanCanValues readNissanDataFromCan(mcp2515_can can) {
  unsigned char len = 0;
  unsigned char buf[8];

  // Hard set of MIL after boot
  if (millis() < 5000) {
    latestNissanCanValues.checkEngineLightState = 2;
  } else {
    latestNissanCanValues.checkEngineLightState = 0;
  }

  if (CAN_MSGAVAIL == can.checkReceive()) {
    can.readMsgBuf(&len, buf);

    unsigned long canId = can.getCanId();

    // Get the current coolant temperature
    if (canId == 0x551) {
      latestNissanCanValues.engineTempCelsius = buf[0] - 40;
      }
    // Read any responses that are from queries sent to the ECM
    else if (canId == 0x7E8) {
      if (buf[0] == 0x04 && buf[1] == 0x62 && buf[2] == 0x11 && buf[3] == 0x1F){ // Oil temperature
        latestNissanCanValues.oilTempCelcius = buf[4] - 50;
      }
      else if (buf[0] == 0x04 && buf[1] == 0x62 && buf[2] == 0x11 && buf[3] == 0x03){ // Battery voltage
        float batteryVoltage = (buf[4] * 0.0196 + 7.84); // Pretty sure there is a better way of getting our values here ?
      }
      else if (buf[0] == 0x05 && buf[1] == 0x62 && buf[2] == 0x12 && buf[3] == 0x0D){ // Gas pedal position
        int raw_value = (buf[4] << 8) | buf[5];
        float voltage = raw_value / 200.0;
        int gasPedalPercentage = ((voltage - gasPedalMinVoltage) / gasPedalVoltageRange) * 100;
        latestNissanCanValues.gasPedalPercentage = gasPedalPercentage;
      }
      else if (buf[0] == 0x05 && buf[1] == 0x62 && buf[2] == 0x12 && buf[3] == 0x25){ // AF Ratio bank 1
        float airFuelRatioBank1 = buf[5];
        latestNissanCanValues.airFuelRatioBank1 = airFuelRatioBank1;
        // SERIAL_PORT_MONITOR.print("Got value for af ratio ");
        // SERIAL_PORT_MONITOR.print(buf[4], HEX);
        // SERIAL_PORT_MONITOR.print("  ");
        // SERIAL_PORT_MONITOR.println(buf[5], HEX);
      }
    }
  }

  // Return the values
  return latestNissanCanValues;
}

/*****************************************************
 *
 * Function - Read latest values from BMW CAN
 *
 ****************************************************/
bmwCanValues bmwCanData; // Holds the data to return to caller of function

float vehicleSpeed;
float wheelSpeedFl = 0;
float wheelSpeedFr = 0;
// float wheelSpeedRl = 0;
// float wheelSpeedRr = 0;

bmwCanValues readBmwDataFromCan(mcp2515_can can) {
  unsigned char len = 0;
  unsigned char buf[8];

  if (CAN_MSGAVAIL == can.checkReceive()) {
    can.readMsgBuf(&len, buf);

    unsigned long canId = can.getCanId();

    // Get the current vehicle wheel speeds
    if (canId == 0x1F0) {
      wheelSpeedFl = (buf[0] + (buf[1] & 15) * 256) / 16.0;
      wheelSpeedFr = (buf[2] + (buf[3] & 15) * 256) / 16.0;
      // wheelSpeedRl = (buf[4] + (buf[5] & 15) * 256) / 16.0;
      // wheelSpeedRr = (buf[6] + (buf[7] & 15) * 256) / 16.0;
    }

    // Calculate the average speed from front wheels and use this as overall vehicle speed
    // NOTE: Will want to change this on a dyno to rear wheels as some ECM tables are speed based
    bmwCanData.vehicleSpeed = ((wheelSpeedFl + wheelSpeedFr) / 2);
    bmwCanData.timestamp = millis();
  }
  return bmwCanData;
}