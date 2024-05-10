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
#include "gearCalculation.h"
#include "globalHelpers.h"

#define CAN_2515

/* ======================================================================
   VARIABLES: Debug and stat output
   ====================================================================== */
bool debugSerialReceive = false;
bool debugSerialSend = false;
bool debugGeneral = false;
bool debugGears = true;

bool reportArduinoLoopStats = false;

/* ======================================================================
   VARIABLES: Pin constants
   ====================================================================== */
const int SPI_SS_PIN_BMW = 9;     // Slave select pin for CAN shield 1 (BMW CAN bus)
const int SPI_SS_PIN_NISSAN = 10; // Slave select pin for CAN shield 2 (Nissan CAN bus)
const int CAN_INT_PIN = 2;

const byte rpmSignalPin = 3;          // Digital input pin for signal wire and interrupt (from Nissan ECU)
const byte fanDriverPwmSignalPin = 6; // Digital output pin for PWM signal to radiator fan motor driver board
const byte alarmBuzzerPin = 5;

// Additional Uno pins assigned in globalHelpers.cpp for multiplexer board 7, 8, 15, 16 and A0
// Temp sensor uses I2C comms on Uno pins 18 and 19
// Ethernet shield uses Uno pin 4 for slave select (bent to the side and jumpered to pin 10 on shield as conflicts with CAN shield)
// Ethernet shield uses SPI comms on Uno pins 11, 12 and 13

/* ======================================================================
   VARIABLES: Mux channel constants
   ====================================================================== */
const byte gaugeOilPressureMuxChannel = 0;        // (Grey)
const byte gaugeOilTemperatureMuxChannel = 1;     // (Brown, uses voltage divider 1.5k ohm)
const byte gaugeCrankCaseVacuumMuxChannel = 2;    // (Red)
const byte gaugeRadiatorOutletTempMuxChannel = 3; // (White, uses voltage divider 1.5k ohm)
const byte gaugeFuelPressureMuxChannel = 4;       // (Black)
const byte clutchSwitchMuxChannel = 5;
const byte neutralSwitchMuxChannel = 6;

/* ======================================================================
   OBJECTS: Define CAN shield objects
   ====================================================================== */
mcp2515_can CAN_BMW(SPI_SS_PIN_BMW);
mcp2515_can CAN_NISSAN(SPI_SS_PIN_NISSAN);

/* ======================================================================
   VARIABLES: General use / functional
   ====================================================================== */
// Define variables for CAN polling behaviour
bool pollEcmCanMetrics = false;
bool pollEcmCanFaults = false;
bool ecmQuerySetupPerformed = false; // Have we sent the setup payloads to ECM to allow us to query various params

// Define variables for current states
float currentAfRatioBank1;
float currentAfRatioBank2;
int currentAirIntakeTemp;
float currentBatteryVoltage;
float currentCrankCaseVacuumPsi;
float currentEngineElectronicsTemp;
float currentFuelPressurePsi;
float currentOilPressurePsi;
float currentOilTempSensor;
float currentRadiatorOutletTemp;
float currentVehicleSpeedFront;
float currentVehicleSpeedRear;
float currentVehicleSpeedRearVariation;
int currentAlphaPercentageBank1;
int currentAlphaPercentageBank2;
int currentCheckEngineLightState = 2; // Setting initial state so check engine light illuminates as soon as possible when powered on
int currentEngineTempCelsius;
int currentFanDutyPercentage;
int currentGasPedalPosition;
int currentOilTempEcm;
int currentRpm = 0;
int currentGear = 0;
unsigned long currentVehicleSpeedTimestamp;

bool clutchPressed = true;
bool inNeutral = true;

// Define constants for different alarm states
const float alarmCrankCaseVacuumPsi = -5;
const int alarmEngineTempCelcius = 110;
const int alarmFuelPressureLowPsi = 48;
const int alarmFuelPressureHighPsi = 57;
const int alarmOilPressurePsi = 10;
const int alarmOilTempCelcius = 120;

// Define misc variables
int consumptionValue = 10;
int setupRetriesMax = 3;            // The number of times we should loop with delay to configure shields etc
float atmospheric_voltage = 0.5707; // The pressure sensor voltage before we start the car, this is 0psi
unsigned long arduinoLoopExecutionCount = 0;

// Define misc CAN payload object
unsigned char canPayloadMisc[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // Check light, fuel consumption and temp alarm light

// Create the MCP9808 temperature sensor object used in the ECU compartment temp measurement
Adafruit_MCP9808 tempSensorEngineElectronics = Adafruit_MCP9808();

/* ======================================================================
   FUNCTION: Write misc payloads to BMW CAN
   ====================================================================== */
void canWriteMisc() {
  canPayloadMisc[0] = currentCheckEngineLightState;       // 2 for check engine light
                                                          // 16 for EML light
                                                          // 18 for check engine AND EML
                                                          // 0 for neither
  canPayloadMisc[1] = consumptionValue;                   // Fuel consumption LSB
  canPayloadMisc[2] = (consumptionValue >> 8);            // Fuel consumption MSB
  if (currentEngineTempCelsius >= alarmEngineTempCelcius) // Set the red alarm light on the temp gauge if needed
    canPayloadMisc[3] = 8;
  else
    canPayloadMisc[3] = 0;

  CAN_BMW.sendMsgBuf(0x545, 0, 8, canPayloadMisc);
}

/* ======================================================================
   OBJECTS: Pretty tiny scheduler objects / tasks
   ====================================================================== */
// High frequency tasks
ptScheduler ptCanWriteMisc = ptScheduler(PT_TIME_10MS);
ptScheduler ptCanWriteRpm = ptScheduler(PT_TIME_10MS);
ptScheduler ptCanWriteSpeed = ptScheduler(PT_TIME_20MS);
ptScheduler ptCanWriteTemp = ptScheduler(PT_TIME_10MS);
ptScheduler ptCanRequestAirFuelRatioBank1 = ptScheduler(PT_TIME_50MS);
ptScheduler ptCanRequestAirFuelRatioBank2 = ptScheduler(PT_TIME_50MS);
ptScheduler ptCanRequestAlphaPercentageBank1 = ptScheduler(PT_TIME_50MS);
ptScheduler ptCanRequestAlphaPercentageBank2 = ptScheduler(PT_TIME_50MS);

// Medium frequency tasks
ptScheduler ptAreWeInAlarmState = ptScheduler(PT_TIME_500MS);
ptScheduler ptCalculateRpm = ptScheduler(PT_TIME_200MS);
ptScheduler ptCanRequestGasPedalPercentage = ptScheduler(PT_TIME_100MS);
ptScheduler ptGaugeReadValueCrankCaseVacuum = ptScheduler(PT_TIME_100MS);
ptScheduler ptGaugeReadValueOilPressure = ptScheduler(PT_TIME_100MS);
ptScheduler ptPublishMqttData100Ms = ptScheduler(PT_TIME_100MS);
ptScheduler ptGetCurrentClutchNeutralAndGear = ptScheduler(PT_TIME_100MS);

// Low frequency tasks
ptScheduler ptCanRequestBatteryVoltage = ptScheduler(PT_TIME_1S);
ptScheduler ptCanRequestOilTemp = ptScheduler(PT_TIME_1S);
ptScheduler ptCanWriteDiagnosticKeepalive = ptScheduler(PT_TIME_1S);
ptScheduler ptGaugeReadValueFuelPressure = ptScheduler(PT_TIME_1S);
ptScheduler ptCanRequestEcmAirIntakeTemp = ptScheduler(PT_TIME_1S);
ptScheduler ptLogNissanCanQueryData = ptScheduler(PT_TIME_1S);
ptScheduler ptReportArduinoLoopStats = ptScheduler(PT_TIME_5S);
ptScheduler ptGaugeReadValueRadiatorOutletTemp = ptScheduler(PT_TIME_5S);
ptScheduler ptGaugeReadValueOilTemp = ptScheduler(PT_TIME_5S);
ptScheduler ptPublishMqttData1S = ptScheduler(PT_TIME_1S);
ptScheduler ptReadEngineElectronicsTemp = ptScheduler(PT_TIME_5S);
ptScheduler ptSetRadiatorFanOutput = ptScheduler(PT_TIME_5S);
ptScheduler ptCanRequestFaults = ptScheduler(PT_TIME_5S);
ptScheduler ptConnectToMqttBroker = ptScheduler(PT_TIME_5S);

/* ======================================================================
   SETUP
   ====================================================================== */
void setup() {
  Serial.begin(115200);
  while (!Serial) {
  };

  // Initialise the multiplexer analogue input board input pin
  setupMux();

  // Initialise ethernet shield
  initialiseEthernetShield();

  // Set the current 0psi voltage of the crank case vacuum sensor if engine is off so that any variation in atmospheric pressure is taken into account
  if (currentRpm == 0) {
    float totalVoltage = 0.0;
    for (int i = 0; i < 10; i++) {
      int sensorValue = analogRead(gaugeCrankCaseVacuumMuxChannel);
      float voltage = sensorValue * (5.0 / 1023.0);
      totalVoltage += voltage;
      delay(20);
    }
    atmospheric_voltage = totalVoltage / 10.0;
    Serial.print("INFO: Initialised crank case vacuum sensor at ");
    Serial.print(atmospheric_voltage);
    Serial.println(" V");
  }

  // Configure CAN shield interfaces
  Serial.println("INFO - Initialising BMW CAN shield");

  for (int i = 0; i < setupRetriesMax; i++) {
    bool result = CAN_BMW.begin(CAN_500KBPS);
    if (result == CAN_OK) {
      Serial.println("\tOK - BMW CAN shield initialised");
      break;
    } else if (result != CAN_OK) {
      Serial.println("\tERROR - BMW CAN shield init failed, retrying ...");
      delay(500);
    }
    if (i == setupRetriesMax) {
      Serial.println("\tFATAL - BMW CAN shield init failed");
    }
  }

  Serial.println("INFO - Initialising Nissan CAN shield");

  for (int i = 0; i < setupRetriesMax; i++) {
    bool result = CAN_NISSAN.begin(CAN_500KBPS);
    if (result == CAN_OK) {
      Serial.println("\tOK - Nissan CAN shield initialised");
      break;
    } else if (result != CAN_OK) {
      Serial.println("\tERROR - Nissan CAN shield init failed, retrying ...");
      delay(500);
    }
    if (i == setupRetriesMax) {
      Serial.println("\tFATAL - Nissan CAN shield init failed");
    }
  }

  // Configure interrupt for RPM signal input
  pinMode(rpmSignalPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(rpmSignalPin), updateRpmPulse, RISING);

  // Configure pins for output to fan controller, clutch swith and neutral switch
  pinMode(fanDriverPwmSignalPin, OUTPUT);

  // Configure the temperature sensor located in the ECU compartment
  Serial.println("INFO - Initialising MCP9808 temperature sensor");

  for (int i = 0; i < setupRetriesMax; i++) {
    bool result = tempSensorEngineElectronics.begin(0x18);
    if (result == 1) {
      Serial.println("\tOK - MCP9808 temperature sensor initialised");
      tempSensorEngineElectronics.setResolution(3);
      break;
    } else if (result != 1) {
      Serial.println("\tERROR - MCP9808 temperature sensor init failed, retrying ...");
      delay(500);
    }
    if (i == setupRetriesMax) {
      Serial.print("\tFATAL - MCP9808 temperature sensor not found, returned status ");
      Serial.println(result);
    }
  }

  /*
  Configure masks and filters for shields to reduce noise. There are two masks in the mcp2515 which both need to be
  set. Mask 0 has 2 filters and mask 1 has 4 so we set them all as needed.
  0x551 is where coolant temperature is located and 0x7E8 is for results of queried CAN parameters.
  */
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

  // Perform short beep to ensure its working on startup
  tone(alarmBuzzerPin, 4000, 1500);
  delay(1500);
}

/* ======================================================================
   MAIN LOOP
   ====================================================================== */
void loop() {
  // Wait until we are sure the ECM is online and publishing data before we call to setup for queried data`
  if (ecmQuerySetupPerformed == false && currentEngineTempCelsius != 0) {
    initialiseEcmForQueries(CAN_NISSAN);
    ecmQuerySetupPerformed = true;
  }

  // Get the clutch, neutral status and determine the current gear from speed and rpm
  if (ptGetCurrentClutchNeutralAndGear.call()) {
    clutchPressed = getClutchStatus(clutchSwitchMuxChannel);
    inNeutral = getNeutralStatus(neutralSwitchMuxChannel);
    currentGear = getCurrentGear(&currentRpm, &currentVehicleSpeedRear, &clutchPressed, &inNeutral);
  }

  // Execute main tasks based on defined timers
  if (ptCalculateRpm.call()) {
    detachInterrupt(digitalPinToInterrupt(rpmSignalPin));
    currentRpm = calculateRpm();
    attachInterrupt(digitalPinToInterrupt(rpmSignalPin), updateRpmPulse, RISING);
    // Change frequency of rpm calculation depending on RPM to avoid high error at low pulse counts
    if (currentRpm >= 1500 && ptCalculateRpm.sequenceList[0] != PT_TIME_50MS) {
      ptCalculateRpm.sequenceList[0] = PT_TIME_50MS;
    } else if (currentRpm < 1500 && ptCalculateRpm.sequenceList[0] != PT_TIME_200MS) {
      ptCalculateRpm.sequenceList[0] = PT_TIME_200MS;
    }
  }

  if (ptCanWriteRpm.call()) {
    canWriteRpm(currentRpm, CAN_BMW);
  }

  if (ptCanWriteTemp.call()) {
    canWriteTemp(currentEngineTempCelsius, CAN_BMW);
  }

  if (ptCanWriteSpeed.call()) {
    canWriteSpeed(currentVehicleSpeedRear, CAN_NISSAN);
  }

  if (ptCanWriteMisc.call()) {
    canWriteMisc();
  }

  if (ptCanWriteDiagnosticKeepalive.call()) {
    canWriteDiagnosticKeepalive(CAN_NISSAN);
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

  // Request CAN data based on defined timers
  if (ptCanRequestFaults.call() && pollEcmCanFaults == true) {
    requestEcmDataFaults(CAN_NISSAN);
  }

  if (ptCanRequestOilTemp.call() && pollEcmCanMetrics == true) {
    requestEcmDataOilTemp(CAN_NISSAN);
  }

  if (ptCanRequestBatteryVoltage.call() && pollEcmCanMetrics == true) {
    requestEcmDataBatteryVoltage(CAN_NISSAN);
  }

  if (ptCanRequestGasPedalPercentage.call() && pollEcmCanMetrics == true) {
    requestEcmDataGasPedalPercentage(CAN_NISSAN);
  }

  if (ptCanRequestAirFuelRatioBank1.call() && pollEcmCanMetrics == true) {
    requestEcmDataAfRatioBank1(CAN_NISSAN);
  }

  if (ptCanRequestAirFuelRatioBank2.call() && pollEcmCanMetrics == true) {
    requestEcmDataAfRatioBank2(CAN_NISSAN);
  }

  if (ptCanRequestAirFuelRatioBank1.call() && pollEcmCanMetrics == true) {
    requestEcmDataAlphaPercentageBank1(CAN_NISSAN);
  }

  if (ptCanRequestAirFuelRatioBank2.call() && pollEcmCanMetrics == true) {
    requestEcmDataAlphaPercentageBank2(CAN_NISSAN);
  }

  if (ptCanRequestEcmAirIntakeTemp.call() && pollEcmCanMetrics == true) {
    requestEcmDataAirIntakeTemp(CAN_NISSAN);
  }

  // Request physical sensor values based on defined timers
  if (ptGaugeReadValueOilPressure.call()) {
    currentOilPressurePsi = gaugeReadPressurePsi(gaugeOilPressureMuxChannel);
  }

  if (ptGaugeReadValueFuelPressure.call()) {
    currentFuelPressurePsi = gaugeReadPressurePsi(gaugeFuelPressureMuxChannel);
  }

  if (ptGaugeReadValueRadiatorOutletTemp.call()) {
    currentRadiatorOutletTemp = gaugeReadTemperatureCelcius(gaugeRadiatorOutletTempMuxChannel);
  }

  if (ptGaugeReadValueOilTemp.call()) {
    currentOilTempSensor = gaugeReadTemperatureCelcius(gaugeOilTemperatureMuxChannel);
  }

  if (ptGaugeReadValueCrankCaseVacuum.call()) {
    currentCrankCaseVacuumPsi = gaugeReadVacuumPsi(gaugeCrankCaseVacuumMuxChannel, atmospheric_voltage);
  }

  // Publish data for Grafana Live consumption via defined timers and over MQTT
  if (ptPublishMqttData100Ms.call()) {
    publishMqttMetric("rpm", "value", currentRpm);
    publishMqttMetric("speed", "value", currentVehicleSpeedFront);
    publishMqttMetric("gear", "value", currentGear);
    publishMqttMetric("diffSpeedSplit", "value", currentVehicleSpeedRearVariation);
    publishMqttMetric("oilPressure", "value", String(currentOilPressurePsi));
    publishMqttMetric("crankCaseVacuum", "value", String(currentCrankCaseVacuumPsi));
    // publishMqttMetric("gasPedalPercent", "value", currentGasPedalPosition);
    // publishMqttMetric("afRatioBank1", "value", String(currentAfRatioBank1));
    // publishMqttMetric("afRatioBank2", "value", String(currentAfRatioBank2));
    // publishMqttMetric("alphaPercentageBank1", "value", currentAlphaPercentageBank1);
    // publishMqttMetric("alphaPercentageBank2", "value", currentAlphaPercentageBank2);
  }

  if (ptPublishMqttData1S.call()) {
    publishMqttMetric("fuelPressure", "value", String(currentFuelPressurePsi));
    publishMqttMetric("coolant", "value", currentEngineTempCelsius);
    publishMqttMetric("ecm", "value", currentEngineElectronicsTemp);
    publishMqttMetric("fan", "value", currentFanDutyPercentage);
    publishMqttMetric("oilTempSensor", "value", currentOilTempSensor);
    publishMqttMetric("radiatorTemp", "value", currentRadiatorOutletTemp);
    publishMqttMetric("0to50", "value", String(getBestZeroToFifty() / 1000));
    publishMqttMetric("0to100", "value", String(getBestZeroToOneHundred() / 1000));
    publishMqttMetric("80to120", "value", String(getBestEightyToOneTwenty() / 1000));
    // publishMqttMetric("oilTempEcm", "value", currentOilTempEcm);
    // publishMqttMetric("airIntakeTemp", "value", currentAirIntakeTemp);
    // publishMqttMetric("batteryVoltage", "value", String(currentBatteryVoltage));
  }

  // Set or unset audible alarm state if conditions are met
  if (ptAreWeInAlarmState.call()) {
    if (currentOilTempEcm > alarmOilTempCelcius || currentEngineTempCelsius > alarmEngineTempCelcius ||
        currentOilPressurePsi < alarmOilPressurePsi || currentCrankCaseVacuumPsi < alarmCrankCaseVacuumPsi ||
        currentFuelPressurePsi < alarmFuelPressureLowPsi || currentFuelPressurePsi > alarmFuelPressureHighPsi) {
      alarmEnable(alarmBuzzerPin, currentRpm);
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
  currentOilTempEcm = currentNissanCanValues.oilTempCelcius;
  currentBatteryVoltage = currentNissanCanValues.batteryVoltage;
  currentGasPedalPosition = currentNissanCanValues.gasPedalPercentage;
  currentAfRatioBank1 = currentNissanCanValues.airFuelRatioBank1;
  currentAfRatioBank2 = currentNissanCanValues.airFuelRatioBank2;
  currentAlphaPercentageBank1 = currentNissanCanValues.alphaPercentageBank1;
  currentAlphaPercentageBank2 = currentNissanCanValues.alphaPercentageBank2;
  currentCheckEngineLightState = currentNissanCanValues.checkEngineLightState;
  currentAirIntakeTemp = currentNissanCanValues.airIntakeTemp;

  if (ptLogNissanCanQueryData.call() && 1 == 2) {
    Serial.print("Engine temp: ");
    Serial.println(currentEngineTempCelsius);
    Serial.print("Oil temp: ");
    Serial.println(currentOilTempEcm);
    Serial.print("Battery voltage: ");
    Serial.println(currentBatteryVoltage);
    Serial.print("Pedal position: ");
    Serial.println(currentGasPedalPosition);
    Serial.print("AFR Bank 1: ");
    Serial.println(currentAfRatioBank1);
    Serial.print("AFR Bank 2: ");
    Serial.println(currentAfRatioBank2);
    Serial.print("Alpha Percentage Bank 1: ");
    Serial.println(currentAlphaPercentageBank1);
    Serial.print("Alpha Percentage Bank 2: ");
    Serial.println(currentAlphaPercentageBank2);
    Serial.print("Check light status: ");
    Serial.println(currentCheckEngineLightState);
    Serial.print("Air intake temp: ");
    Serial.println(currentAirIntakeTemp);
  }

  // Pull the values were are interested in from the BMW CAN response
  currentVehicleSpeedFront = currentBmwCanValues.vehicleSpeedFront;
  currentVehicleSpeedRear = currentBmwCanValues.vehicleSpeedRear;
  currentVehicleSpeedRearVariation = currentBmwCanValues.vehicleSpeedRearVariation;
  currentVehicleSpeedTimestamp = currentBmwCanValues.timestamp;

  // Pass the current speed and timestamp values into functions for performance metrics
  captureAccellerationTimes(currentVehicleSpeedTimestamp, currentVehicleSpeedFront);
  // captureAccellerationDetailedData(currentVehicleSpeedTimestamp, currentVehicleSpeed);

  // Increment loop counter if needed so we can report on stats (806 Hz for Mega and 4550 Hz for Uno R4)
  if (millis() > 10000 && reportArduinoLoopStats) {
    arduinoLoopExecutionCount++;
    if (ptReportArduinoLoopStats.call()) {
      reportArduinoLoopRate(&arduinoLoopExecutionCount);
    }
  }
}
