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
        } else if (canId == 0x180) {
            if (bitRead(buf[7], 4) == 1) {                      // Bit 4 contains check engine light status ie: 0 0 0 1 0 0 0 0
                                                                // where check light is off when the bit is set
                nissanCanData.checkEngineLightState = 0;
            }
            if (bitRead(buf[7], 4) == 0) {                      // Bit 4 is not set so check light needs to be set on
                nissanCanData.checkEngineLightState = 2;
            }
        }
        
        // This is for debug only to capture all Nissan side CAN data (based on active filters)
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