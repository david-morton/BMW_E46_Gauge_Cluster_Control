/* This is a basic program to drive the E46 gauge cluster via CAN. The hardware
used was a single Arduino Mega 2560 r3 and two Seeed CAN-Bus shield v2's.

This project was used in the context of an engine swap where a Nissan VQ37VHR
was installed into the E46, and the desire was to maintain a functional OEM
dashboard.

The scaling ratio of engine RPM to the hex value of CAN payload is
approximated in this program based on a linear formula worked out in Excel.

This was done from a test cluster so that visually the needle lined up
as well as possible throughout the RPM range. Individual clusters may vary
so some experimentation with this may be desired ... practically speaking it
may make no differance at all :)

For reference the measured ratios at varios RPM's in my own test cluster are as per the below:
1000 6.65
2000 6.59
3000 6.53
4000 6.48
5000 6.42
6000 6.37
7000 6.31

The fuel economy gauge only works when there is a speed input. Inputting a
square wave of 410Hz and 50% duty cycle gives 60km/h. This signal is put into
pin 19 on cluster connector X11175 for testing purposes on the bench. This
signal is usually provided by the ABS computer.

The lowest speed pulse generation that seems to allow activation of the fuel
economy gauge is 82Hz when increasing and it will stop function at 68Hz when
decreasing. 82Hz seems to be about 5kph.

VQ37 ECU Pin 110 is 'Engine speed output signal' and outputs a square wave at 3
pulses per revolution. We use this signal to calculate the engine RPM and send
the value to the cluster.

The Arduino will also be used to control the radiator thermo fan via a Cytron
MD30C. This is a PWM motor controller suitable for brushed DC motors up to a
constant 30A.
*/

#include <Adafruit_MCP9808.h> // Used for temperature sensor
#include <Arduino.h>
#include <mcp2515_can.h> // Used for Seeed CAN shields
#include <ptScheduler.h> // The task scheduling library of choice

#include "functions_analogue_gauges.h"
#include "functions_do.h"
#include "functions_mqtt.h"
#include "functions_performance.h"
#include "functions_poll_ecm.h"
#include "functions_read.h"
#include "functions_write.h"

#define CAN_2515

// Some pin assignments. In addition temp sensor uses 20 and 21, ethernet uses 31 for slave select
const int SPI_SS_PIN_BMW = 9;     // Slave select pin for CAN shield 1 (BMW CAN bus)
const int SPI_SS_PIN_NISSAN = 10; // Slave select pin for CAN shield 2 (Nissan CAN bus)
const int CAN_INT_PIN = 2;

const byte rpmSignalPin = 19;                // Digital input pin for signal wire and interrupt (from Nissan ECU)
const byte fanDriverPwmSignalPin = 46;       // Digital output pin for PWM signal to radiator fan motor driver board
const byte gaugeOilPressurePin = A8;         // Analogue pin 8
const byte gaugeFuelPressurePin = A9;        // Analogue pin 9
const byte gaugeCrankCaseVacuumPin = A10;    // Analogue pin 10
                                             // Analogue pin 11 apparently used internaly for tone generation purposes
const byte gaugeRadiatorOutletTempPin = A12; // Analogue pin 12
const byte alarmBuzzerPin = 39;

// Define CAN objects
mcp2515_can CAN_BMW(SPI_SS_PIN_BMW);
mcp2515_can CAN_NISSAN(SPI_SS_PIN_NISSAN);

// Define variables for current states
float currentAfRatioBank1;
float currentAfRatioBank2;
float currentBatteryVoltage;
float currentCrankCaseVacuumBar;
float currentEngineElectronicsTemp;
float currentFuelPressurePsi;
float currentOilPressurePsi;
float currentRadiatorOutletTemp;
float currentVehicleSpeed;
int currentAlphaPercentageBank1;
int currentAlphaPercentageBank2;
int currentCheckEngineLightState;
int currentClutchStatus;
int currentEngineTempCelsius;
int currentFanDutyPercentage;
int currentGasPedalPosition;
int currentOilTempCelcius;
int currentRpm;
unsigned long currentVehicleSpeedTimestamp;

// Define alarm state variables
const float alarmCrankCaseVacuumBar = -0.2;
const int alarmEngineTempCelcius = 115;
const int alarmFuelPressurePsi = 48;
const int alarmOilPressurePsi = 12;
const int alarmOilTempCelcius = 15;

// Define other variables
int consumptionValue = 10;
int setupRetriesMax = 3;             // The number of times we should loop with delay to configure shields etc
const int tempAlarmLight = 110;      // What temperature should the warning light come on at
bool ecmQuerySetupPerformed = false; // Have we sent the setup payloads to ECM to allow us to query various params
bool inAlarmState = false;           // Are we currently in alarm state making a big noise ?

// Define misc CAN payload
unsigned char canPayloadMisc[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // Check light, fuel consumption and temp alarm light

// Create the MCP9808 temperature sensor object
Adafruit_MCP9808 tempSensorEngineElectronics = Adafruit_MCP9808();

// Function - Write misc payload to BMW CAN
void canWriteMisc() {
  canPayloadMisc[0] = currentCheckEngineLightState; // 2 for check engine light
                                                    // 16 for EML light
                                                    // 18 for check engine AND EML
                                                    // 0 for neither
  canPayloadMisc[1] = consumptionValue;             // Fuel consumption LSB
  canPayloadMisc[2] = (consumptionValue >> 8);      // Fuel consumption MSB
  if (currentEngineTempCelsius >= tempAlarmLight)   // Set the red alarm light on the temp gauge if needed
    canPayloadMisc[3] = 8;
  else
    canPayloadMisc[3] = 0;

  CAN_BMW.sendMsgBuf(0x545, 0, 8, canPayloadMisc);
}

// Define our pretty tiny scheduler objects
ptScheduler ptAreWeInAlarmState = ptScheduler(PT_TIME_500MS);
ptScheduler ptCalculateRpm = ptScheduler(PT_TIME_50MS);
ptScheduler ptCanRequestAirFuelRatioBank1 = ptScheduler(PT_TIME_100MS);
ptScheduler ptCanRequestAirFuelRatioBank2 = ptScheduler(PT_TIME_100MS);
ptScheduler ptCanRequestAlphaPercentageBank1 = ptScheduler(PT_TIME_100MS);
ptScheduler ptCanRequestAlphaPercentageBank2 = ptScheduler(PT_TIME_100MS);
ptScheduler ptCanRequestBatteryVoltage = ptScheduler(PT_TIME_1S);
ptScheduler ptCanRequestGasPedalPercentage = ptScheduler(PT_TIME_100MS);
ptScheduler ptCanRequestOilTemp = ptScheduler(PT_TIME_1S);
ptScheduler ptCanWriteClutchStatus = ptScheduler(PT_TIME_100MS); // Can probably be removed, may need manual ECM to support ?
ptScheduler ptCanWriteMisc = ptScheduler(PT_TIME_10MS);
ptScheduler ptCanWriteRpm = ptScheduler(PT_TIME_10MS);
ptScheduler ptCanWriteSpeed = ptScheduler(PT_TIME_20MS);
ptScheduler ptCanWriteTemp = ptScheduler(PT_TIME_10MS);
ptScheduler ptConnectToMqttBroker = ptScheduler(PT_TIME_9S);
ptScheduler ptGaugeReadValueCrankCaseVacuum = ptScheduler(PT_TIME_100MS);
ptScheduler ptGaugeReadValueFuelPressure = ptScheduler(PT_TIME_1S);
ptScheduler ptGaugeReadValueOilPressure = ptScheduler(PT_TIME_100MS);
ptScheduler ptGaugeReadValueRadiatorOutletTemp = ptScheduler(PT_TIME_5S);
ptScheduler ptPublishMqttData100Ms = ptScheduler(PT_TIME_100MS);
ptScheduler ptPublishMqttData1S = ptScheduler(PT_TIME_1S);
ptScheduler ptPublishMqttData5S = ptScheduler(PT_TIME_5S);
ptScheduler ptReadEngineElectronicsTemp = ptScheduler(PT_TIME_5S);
ptScheduler ptSetRadiatorFanOutput = ptScheduler(PT_TIME_5S);

// Perform one time setup pieces
void setup() {
  SERIAL_PORT_MONITOR.begin(115200);
  while (!Serial) {
  };

  // Initialise ethernet shield and mqtt client
  initialiseEthernetShield();

  // Configure CAN shield interfaces
  SERIAL_PORT_MONITOR.println("INFO - Initialising BMW CAN shield");

  for (int i = 0; i < setupRetriesMax; i++) {
    bool result = CAN_BMW.begin(CAN_500KBPS);
    if (result == CAN_OK) {
      SERIAL_PORT_MONITOR.println("\tOK - BMW CAN shield initialised");
      break;
    } else if (result != CAN_OK) {
      SERIAL_PORT_MONITOR.println("\tERROR - BMW CAN shield init failed, retrying ...");
      delay(500);
    }
    if (i == setupRetriesMax) {
      SERIAL_PORT_MONITOR.println("\tFATAL - BMW CAN shield init failed");
    }
  }

  SERIAL_PORT_MONITOR.println("INFO - Initialising Nissan CAN shield");

  for (int i = 0; i < setupRetriesMax; i++) {
    bool result = CAN_NISSAN.begin(CAN_500KBPS);
    if (result == CAN_OK) {
      SERIAL_PORT_MONITOR.println("\tOK - Nissan CAN shield initialised");
      break;
    } else if (result != CAN_OK) {
      SERIAL_PORT_MONITOR.println("\tERROR - Nissan CAN shield init failed, retrying ...");
      delay(500);
    }
    if (i == setupRetriesMax) {
      SERIAL_PORT_MONITOR.println("\tFATAL - Nissan CAN shield init failed");
    }
  }

  // Configure interrupt for RPM signal input
  pinMode(rpmSignalPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(rpmSignalPin), updateRpmPulse, RISING);

  // Configure pins for output to fan controller
  pinMode(fanDriverPwmSignalPin, OUTPUT);

  // Configure the temperature sensor
  SERIAL_PORT_MONITOR.println("INFO - Initialising MCP9808 temperature sensor");

  for (int i = 0; i < setupRetriesMax; i++) {
    bool result = tempSensorEngineElectronics.begin(0x18);
    if (result == 1) {
      SERIAL_PORT_MONITOR.println("\tOK - MCP9808 temperature sensor initialised");
      tempSensorEngineElectronics.setResolution(3);
      break;
    } else if (result != 1) {
      SERIAL_PORT_MONITOR.println("\tERROR - MCP9808 temperature sensor init failed, retrying ...");
      delay(500);
    }
    if (i == setupRetriesMax) {
      SERIAL_PORT_MONITOR.print("\tFATAL - MCP9808 temperature sensor not found, returned status ");
      SERIAL_PORT_MONITOR.println(result);
    }
  }

  // Configure masks and filters for shields to reduce noise. There are two masks in the mcp2515 which both need to be
  // set. Mask 0 has 2 filters and mask 1 has 4 so we set them all as needed 0x551 is where coolant temperature is
  // located 0x7E8 is for results of queried parameters
  CAN_NISSAN.init_Mask(0, 0, 0xFFF);
  CAN_NISSAN.init_Filt(0, 0, 0x551);
  CAN_NISSAN.init_Filt(1, 0, 0x7E8);

  CAN_NISSAN.init_Mask(1, 0, 0xFFF);
  CAN_NISSAN.init_Filt(2, 0, 0x551);
  CAN_NISSAN.init_Filt(3, 0, 0x7E8);
  CAN_NISSAN.init_Filt(4, 0, 0x551);
  CAN_NISSAN.init_Filt(5, 0, 0x7E8);

  // 0x1F0 is where the individual wheel speeds are
  // https://www.bimmerforums.com/forum/showthread.php?1887229-E46-Can-bus-project
  CAN_BMW.init_Mask(0, 0, 0xFFF);
  CAN_BMW.init_Filt(0, 0, 0x1F0);
  CAN_BMW.init_Filt(1, 0, 0x1F0);

  CAN_BMW.init_Mask(1, 0, 0xFFF);
  CAN_BMW.init_Filt(2, 0, 0x1F0);
  CAN_BMW.init_Filt(3, 0, 0x1F0);
  CAN_BMW.init_Filt(4, 0, 0x1F0);
  CAN_BMW.init_Filt(5, 0, 0x1F0);
}

// Our main loop
void loop() {
  // Wait until we are sure the ECM is online and publishing data before we call to setup for queried data
  if (ecmQuerySetupPerformed == false && currentEngineTempCelsius != 0) {
    initialiseEcmForQueries(CAN_NISSAN);
    ecmQuerySetupPerformed = true;
  }

  // Check the pretty tiny scheduler tasks and call as needed
  if (ptCalculateRpm.call()) {
    currentRpm = calculateRpm();
  }

  if (ptCanWriteRpm.call()) {
    canWriteRpm(currentRpm, CAN_BMW);
  }

  if (ptCanWriteTemp.call()) {
    canWriteTemp(currentEngineTempCelsius, CAN_BMW);
  }

  if (ptCanWriteSpeed.call()) {
    canWriteSpeed(currentVehicleSpeed, CAN_NISSAN);
  }

  if (ptCanWriteClutchStatus.call()) {
    canWriteClutchStatus(currentClutchStatus, CAN_NISSAN);
  }

  if (ptCanWriteMisc.call()) {
    canWriteMisc();
  }

  if (ptSetRadiatorFanOutput.call()) {
    currentFanDutyPercentage = setRadiatorFanOutput(currentEngineTempCelsius, currentRpm, fanDriverPwmSignalPin);
  }

  if (ptReadEngineElectronicsTemp.call()) {
    currentEngineElectronicsTemp = readEngineElectronicsTemp(tempSensorEngineElectronics);
  }

  if (ptConnectToMqttBroker.call()) {
    connectMqttClientToBroker();
  }

  if (ptCanRequestOilTemp.call()) {
    requestEcmDataOilTemp(CAN_NISSAN);
  }

  if (ptCanRequestBatteryVoltage.call()) {
    requestEcmDataBatteryVoltage(CAN_NISSAN);
  }

  if (ptCanRequestGasPedalPercentage.call()) {
    requestEcmDataGasPedalPercentage(CAN_NISSAN);
  }

  if (ptCanRequestAirFuelRatioBank1.call()) {
    requestEcmDataAfRatioBank1(CAN_NISSAN);
  }

  if (ptCanRequestAirFuelRatioBank2.call()) {
    requestEcmDataAfRatioBank2(CAN_NISSAN);
  }

  if (ptCanRequestAirFuelRatioBank1.call()) {
    requestEcmDataAlphaPercentageBank1(CAN_NISSAN);
  }

  if (ptCanRequestAirFuelRatioBank2.call()) {
    requestEcmDataAlphaPercentageBank2(CAN_NISSAN);
  }

  if (ptGaugeReadValueOilPressure.call()) {
    currentOilPressurePsi = gaugeReadPressurePsi(gaugeOilPressurePin);
  }

  if (ptGaugeReadValueFuelPressure.call()) {
    currentFuelPressurePsi = gaugeReadPressurePsi(gaugeFuelPressurePin);
  }

  if (ptGaugeReadValueRadiatorOutletTemp.call()) {
    currentRadiatorOutletTemp = gaugeReadTemperatureCelcius(gaugeRadiatorOutletTempPin);
  }

  if (ptGaugeReadValueCrankCaseVacuum.call()) {
    currentCrankCaseVacuumBar = gaugeReadVacuumBar(gaugeCrankCaseVacuumPin);
  }

  if (ptPublishMqttData100Ms.call()) {
    publishMqttMetric("rpm", "value", currentRpm);
    publishMqttMetric("speed", "value", currentVehicleSpeed);
    publishMqttMetric("gasPedalPercent", "value", currentGasPedalPosition);
    publishMqttMetric("afRatioBank1", "value", String(currentAfRatioBank1));
    publishMqttMetric("afRatioBank2", "value", String(currentAfRatioBank2));
    publishMqttMetric("alphaPercentageBank1", "value", currentAlphaPercentageBank1);
    publishMqttMetric("alphaPercentageBank2", "value", currentAlphaPercentageBank2);
    publishMqttMetric("oilPressure", "value", String(currentOilPressurePsi));
    publishMqttMetric("crankCaseVacuum", "value", String(currentCrankCaseVacuumBar));
  }

  if (ptPublishMqttData1S.call()) {
    publishMqttMetric("batteryVoltage", "value", String(currentBatteryVoltage));
    publishMqttMetric("fuelPressure", "value", String(currentFuelPressurePsi));
  }

  if (ptPublishMqttData5S.call()) {
    publishMqttMetric("coolant", "value", currentEngineTempCelsius);
    publishMqttMetric("ecm", "value", currentEngineElectronicsTemp);
    publishMqttMetric("fan", "value", currentFanDutyPercentage);
    publishMqttMetric("oilTemp", "value", currentOilTempCelcius);
    publishMqttMetric("radiatorTemp", "value", currentRadiatorOutletTemp);
  }

  if (ptAreWeInAlarmState.call()) {
    if (!inAlarmState &&
        (currentOilTempCelcius > alarmOilTempCelcius || currentEngineTempCelsius > alarmEngineTempCelcius)) {
      alarmEnable(alarmBuzzerPin, currentRpm);
      inAlarmState = true;
    } else {
      alarmDisable(alarmBuzzerPin);
    }
  }

  // Fetch the latest values from Nissan CAN
  nissanCanValues currentNissanCanValues = readNissanDataFromCan(CAN_NISSAN);

  // Fetch the latest values from BMW CAN
  bmwCanValues currentBmwCanValues = readBmwDataFromCan(CAN_BMW);

  // Pull the values we are interested in from the Nissan CAN response
  currentEngineTempCelsius = currentNissanCanValues.engineTempCelsius;
  currentOilTempCelcius = currentNissanCanValues.oilTempCelcius;
  currentBatteryVoltage = currentNissanCanValues.batteryVoltage;
  currentGasPedalPosition = currentNissanCanValues.gasPedalPercentage;
  currentAfRatioBank1 = currentNissanCanValues.airFuelRatioBank1;
  currentAfRatioBank2 = currentNissanCanValues.airFuelRatioBank2;
  currentAlphaPercentageBank1 = currentNissanCanValues.alphaPercentageBank1;
  currentAlphaPercentageBank2 = currentNissanCanValues.alphaPercentageBank2;
  currentCheckEngineLightState = currentNissanCanValues.checkEngineLightState;

  // Pull the values were are interested in from the BMW CAN response
  currentVehicleSpeed = currentBmwCanValues.vehicleSpeed;
  currentVehicleSpeedTimestamp = currentBmwCanValues.timestamp;

  // Pass the current speed and timestamp values into functions for performance metrics
  // captureAccellerationTimes(currentVehicleSpeedTimestamp, currentVehicleSpeed);
  // captureAccellerationDetailedData(currentVehicleSpeedTimestamp, currentVehicleSpeed);
}
