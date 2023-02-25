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
// Define our custom struct for holding the last value timestamps
nissanCanUpdateTimes latestNissanCanUpdateTimes;

nissanCanValues latestNissanCanValues; // Holds the data to return to caller of function
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
      latestNissanCanUpdateTimes.engineTempCelsius = millis();
      }
    // Read any responses that are from queries sent to the ECM
    else if (canId == 0x7E8) {
      if (buf[0] == 0x04 && buf[1] == 0x62 && buf[2] == 0x11 && buf[3] == 0x1F)
      {
        latestNissanCanValues.oilTempCelcius = buf[4] - 50;
        latestNissanCanUpdateTimes.oilTempCelcius = micros();
      }
      if (buf[0] == 0x04 && buf[1] == 0x62 && buf[2] == 0x11 && buf[3] == 0x03)
      {
        latestNissanCanValues.batteryVoltage = buf[4];
        latestNissanCanUpdateTimes.batteryVoltage = micros();
      }
      
      // Oil temp looks like 46 hex is 70 and temp was 20 ... 0x7E8   4       62      11      1F      46
      // Battery voltags                                      0x7E8   4       62      11      3       9D
    }
    // Debug to capture all Nissan side CAN data (based on active filters set on the shield)
    // SERIAL_PORT_MONITOR.print("0x");
    // SERIAL_PORT_MONITOR.print(canId, HEX);
    // SERIAL_PORT_MONITOR.print("\t");

    // for (int i = 0; i < len; i++) { // print the data
    //     SERIAL_PORT_MONITOR.print(buf[i], HEX);
    //     SERIAL_PORT_MONITOR.print("\t");
    // }
    // SERIAL_PORT_MONITOR.println();
  }
  // Compare latest timestamps and zero out values if they are deemed too old
  if (latestNissanCanUpdateTimes.engineTempCelsius < (millis() - 10000)) { latestNissanCanValues.engineTempCelsius = 0; }
  if (latestNissanCanUpdateTimes.oilTempCelcius < (millis() - 10000)) { latestNissanCanValues.oilTempCelcius = 0; }
  
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