#include "functions_read.h"
#include <Adafruit_MCP9808.h> // Used for temperature sensor
#include <mcp2515_can.h>      // Used for Seeed shields
#include <map>                // Used for defining the AFR lookup table

/*****************************************************
 *
 * Function - Get the current engine bay electronics temp
 *
 ****************************************************/
float readEngineElectronicsTemp(Adafruit_MCP9808 temp) { return temp.readTempC(); }

/*****************************************************
 *
 * Function - Calculate the air/fuel ratio from voltage
 *
 ****************************************************/
// Define lookup table for air/fuel ratios as measured from UpRev
std::map<int,int> afRatioLookup = {{0,1000},{10,1023},{20,1047},{30,1071},{40,1094},{50,1117},{60,1141},{70,1164},{80,1188},{90,1211},{100,1235},{110,1258},{120,1282},{130,1303},{140,1321},{150,1338},{160,1356},{170,1377},{180,1398},{190,1419},{200,1440},{210,1459},{220,1477},{230,1511},{240,1560},{250,1611},{260,1662},{270,1713},{280,1770},{290,1830},{300,1888},{310,1947},{320,2012},{330,2128},{340,2244},{350,2360},{360,2477},{370,2593},{380,2709},{390,2826},{400,2942},{410,3116},{420,3349},{430,3581},{440,3814},{450,4073},{460,4452},{470,4968},{480,5484},{490,6000}};

float calculateAfRatioFromVoltage(float decimalVoltage){
  if (decimalVoltage == 0) {
    return 10.0;
  } else if (decimalVoltage >= 4.9) {
    return 60.0;
  } else {
    int lookupVoltage = decimalVoltage * 100;
    lookupVoltage = lookupVoltage / 10 * 10; // Truncate the value so we get the next lowest value from our lookup
    auto searchResult = afRatioLookup.find(lookupVoltage);
    int searchAfRatio = searchResult->second;
    int nextHighestAfRatio = afRatioLookup.find(lookupVoltage + 10)->second;
    int afRatioDifferance = nextHighestAfRatio - searchAfRatio;
    float calculatedAfRatio = (searchAfRatio + (((decimalVoltage * 100 - lookupVoltage) / 10) * afRatioDifferance));
    // SERIAL_PORT_MONITOR.print("Decimal of ");
    // SERIAL_PORT_MONITOR.print(decimalVoltage);
    // SERIAL_PORT_MONITOR.print(" means a lookup value of ");
    // SERIAL_PORT_MONITOR.print(lookupVoltage);
    // SERIAL_PORT_MONITOR.print(" which is AFR lookup of ");
    // SERIAL_PORT_MONITOR.print(searchAfRatio);
    // SERIAL_PORT_MONITOR.print(". The next highest reading is ");
    // SERIAL_PORT_MONITOR.print(nextHighestAfRatio);
    // SERIAL_PORT_MONITOR.print(" and the diff is ");
    // SERIAL_PORT_MONITOR.println(afRatioDifferance);
    // SERIAL_PORT_MONITOR.print("The exact multiplied AFR is ");
    // SERIAL_PORT_MONITOR.print(calculatedAfRatio);
    // SERIAL_PORT_MONITOR.println("\n");
    return calculatedAfRatio / 100;
  }
}

/*****************************************************
 *
 * Function - Read latest values from Nissan CAN
 *
 ****************************************************/
// Define min and max gas pedal voltages as measued from UpRev from which we derive percentage
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
      // Oil temperature
      if (buf[0] == 0x04 && buf[1] == 0x62 && buf[2] == 0x11 && buf[3] == 0x1F) {
        latestNissanCanValues.oilTempCelcius = buf[4] - 50;
      // Battery voltage (at the ECM ?? Does not line up with actual battery)
      } else if (buf[0] == 0x04 && buf[1] == 0x62 && buf[2] == 0x11 && buf[3] == 0x03) {
        int raw_value = (buf[3] << 8) | buf[4];
        float batteryVoltage = raw_value / 65.0; // This needs another look !!
        latestNissanCanValues.batteryVoltage = batteryVoltage;
      // Gas pedal position
      } else if (buf[0] == 0x05 && buf[1] == 0x62 && buf[2] == 0x12 && buf[3] == 0x0D) {
        int raw_value = (buf[4] << 8) | buf[5];
        float voltage = raw_value / 200.0;
        int gasPedalPercentage = ((voltage - gasPedalMinVoltage) / gasPedalVoltageRange) * 100;
        latestNissanCanValues.gasPedalPercentage = gasPedalPercentage;
      // Air fuel ratio bank 1
      } else if (buf[0] == 0x05 && buf[1] == 0x62 && buf[2] == 0x12 && buf[3] == 0x25) {
        int raw_value = (buf[4] << 8) | buf[5];
        float airFuelRatioBank1Voltage = raw_value / 200.0;
        latestNissanCanValues.airFuelRatioBank1 = calculateAfRatioFromVoltage(airFuelRatioBank1Voltage);
      // Air fuel ratio bank 2
      } else if (buf[0] == 0x05 && buf[1] == 0x62 && buf[2] == 0x12 && buf[3] == 0x26) {
        int raw_value = (buf[4] << 8) | buf[5];
        float airFuelRatioBank2Voltage = raw_value / 200.0;
        latestNissanCanValues.airFuelRatioBank2 = calculateAfRatioFromVoltage(airFuelRatioBank2Voltage);
      // Alpha percentage bank 1
      } else if (buf[0] == 0x04 && buf[1] == 0x62 && buf[2] == 0x11 && buf[3] == 0x23) {
        int alphaPercentageBank1 = buf[4];
        latestNissanCanValues.alphaPercentageBank1 = alphaPercentageBank1;
      // Alpha percentage bank 1
      } else if (buf[0] == 0x04 && buf[1] == 0x62 && buf[2] == 0x11 && buf[3] == 0x24) {
        int alphaPercentageBank2 = buf[4];
        latestNissanCanValues.alphaPercentageBank2 = alphaPercentageBank2;
      // Air intake temperature
      } else if (buf[0] == 0x04 && buf[1] == 0x62 && buf[2] == 0x11 && buf[3] == 0x06) {
        int airIntakeTemp = buf[4] - 50;
        latestNissanCanValues.airIntakeTemp = airIntakeTemp;
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