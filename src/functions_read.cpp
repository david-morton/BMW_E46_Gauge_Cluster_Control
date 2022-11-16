#include "functions_read.h"
#include <Adafruit_MCP9808.h>   // Used for temperature sensor
#include <mcp2515_can.h>        // Used for Seeed shields

/*****************************************************
 *
 * Function - Get the current engine bay electronics temp
 *
 ****************************************************/
float readEngineElectronicsTemp(Adafruit_MCP9808 t) {
    return t.readTempC();
}

/*****************************************************
 *
 * Function - Read latest values from Nissan CAN
 *
 ****************************************************/
nissanCanValues nissanCanData;                              // Holds the data to return to caller of function
int engineTempCelsius;
int checkEngineLightState;
bool didWeSeeCheckLightOnStart = 0;

nissanCanValues readNissanDataFromCan(mcp2515_can can) {
    unsigned char len = 0;
    unsigned char buf[8];

    if (CAN_MSGAVAIL == can.checkReceive()) {
        can.readMsgBuf(&len, buf);

        unsigned long canId = can.getCanId();

        // Get the current coolant temperature and engine check light state
        if (canId == 0x551) {
            nissanCanData.engineTempCelsius = buf[0] - 40;
        } else if (canId == 0x180) {
            // Debug to look at the bits which are set on a particular byte
            // SERIAL_PORT_MONITOR.println();
            // for (int i = 7; i >= 0; i--)
            // {
            //     bool b = bitRead(buf[7], i);
            //     SERIAL_PORT_MONITOR.print(b);
            // }

            if (bitRead(buf[7], 4) == 1) {                      // Bit 4 contains check engine light status ie: 0 0 0 1 0 0 0 0
                                                                // where check light is off when the bit is set
                nissanCanData.checkEngineLightState = 0;
            }
            if (bitRead(buf[7], 4) == 0) {                      // Bit 4 is not set so check light needs to be set on
                nissanCanData.checkEngineLightState = 2;
                didWeSeeCheckLightOnStart = 1;
            }

            // This is a hacky solution to show the check light on ignition on (providing Arduino has power before ECU)
            // just so that we can have confidance that the code is working. Debug shows it only sends 2 CAN frames with
            // the check light bit not set (light on). Unsure how this is catered for in the factory setup.
            if (millis() < 5000 && didWeSeeCheckLightOnStart == 1) {
                nissanCanData.checkEngineLightState = 2;
            }
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
    return nissanCanData;
}

/*****************************************************
 *
 * Function - Read latest values from BMW CAN
 *
 ****************************************************/
bmwCanValues bmwCanData;     // Holds the data to return to caller of function
int vehicleSpeed;
float wheelSpeedFl = 0;
float wheelSpeedFr = 0;
float wheelSpeedRl = 0;
float wheelSpeedRr = 0;

bmwCanValues readBmwDataFromCan(mcp2515_can can) {
    unsigned char len = 0;
    unsigned char buf[8];

    if (CAN_MSGAVAIL == can.checkReceive()) {
        can.readMsgBuf(&len, buf);

        unsigned long canId = can.getCanId();

        // Get the current vehicle wheel speeds
        if (canId == 0x1F0) {
            // Excel based formula for this is (HEX2DEC(B0)+HEX2DEC(RIGHT(B1,1))*256)/16
            wheelSpeedFl = (buf[0] + (buf[1] & 15) * 256) / 16.0;
            wheelSpeedFr = (buf[2] + (buf[3] & 15) * 256) / 16.0;
            wheelSpeedRl = (buf[4] + (buf[5] & 15) * 256) / 16.0;
            wheelSpeedRr = (buf[6] + (buf[7] & 15) * 256) / 16.0;
        }

        // Calculate the average speed from front wheels and use this as overall vehicle speed
        bmwCanData.vehicleSpeed = (wheelSpeedFl + wheelSpeedFr) / 2;
    }
    return bmwCanData;
}