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
unsigned long engineCheckTriggeredMillis = 1;               // Holds the timestamp when engine check was triggered
const int minimumEngineCheckLightDuration = 3;              // How many seconds should engine check light display for minimum
                                                            // This is to ensure it illuminates at ignition on as it is too fast
                                                            // to show on the BMW cluster. This is only useful if you power the
                                                            // Arduino via accessory power, in my case the check light happens
                                                            // before the Arduino boots up and can push the CAN payload.

nissanCanValues readNissanDataFromCan(mcp2515_can can) {
    unsigned char len = 0;
    unsigned char buf[8];

    if (CAN_MSGAVAIL == can.checkReceive()) {
        can.readMsgBuf(&len, buf);

        unsigned long canId = can.getCanId();

        // Get the current coolant temperature and engine check light state
        if (canId == 0x551) {
            nissanCanData.engineTempCelsius = buf[0] - 40;      // Internet info said -48 from hex byte A but does not line up with 
                                                                // data from NDSIII temperature so -40 it is
        } else if (canId == 0x160) {
            if (buf[6] == 192) {                                // Hex C0, check engine light is off
                if (millis() > (engineCheckTriggeredMillis + (minimumEngineCheckLightDuration * 1000))) {
                    nissanCanData.checkEngineLightState = 0;
                }
            }
            if (buf[6] == 224) {                                // Hex E0, check engine light is on
                nissanCanData.checkEngineLightState = 2;
                engineCheckTriggeredMillis = millis();
            }
        }
        
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