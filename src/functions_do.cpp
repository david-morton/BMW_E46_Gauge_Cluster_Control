#include "functions_do.h"

/****************************************************
 *
 * ISR - Update the RPM counter and time via interrupt
 *
 ****************************************************/
volatile unsigned long latestRpmPulseTime = micros(); // Will store latest ISR micros value for calculations
volatile unsigned long latestRpmPulseCounter = 0;     // Will store latest number of pulses counted

void updateRpmPulse() {
  latestRpmPulseCounter++;
  latestRpmPulseTime = micros();
}

/****************************************************
 *
 * Function - Calculate the current RPM value from pulses measured via ISR
 *
 ****************************************************/
const int rpmPulsesPerRevolution = 3;  // Number of pulses on the signal wire per crank revolution
unsigned long previousRpmPulseTime;    // Will store previous ISR micros value for calculations
unsigned long previousRpmPulseCounter; // Will store previous the number of pulses counted

int calculateRpm() {
  unsigned long deltaMicros = latestRpmPulseTime - previousRpmPulseTime;
  unsigned long deltaRpmPulseCounter = latestRpmPulseCounter - previousRpmPulseCounter;

  float microsPerPulse = deltaMicros / deltaRpmPulseCounter;

  float pulsesPerMinute = 60000000 / microsPerPulse;

  // Set previous variables to current values for next loop
  previousRpmPulseCounter = latestRpmPulseCounter;
  previousRpmPulseTime = latestRpmPulseTime;

  // Do the calcultion
  int currentRpm = pulsesPerMinute / rpmPulsesPerRevolution;

  // Debug infos
  // SERIAL_PORT_MONITOR.print("Micros since last execution is ");
  // SERIAL_PORT_MONITOR.print(deltaMicros);
  // SERIAL_PORT_MONITOR.print(", pulses since last execution is ");
  // SERIAL_PORT_MONITOR.print(deltaRpmPulseCounter);
  // SERIAL_PORT_MONITOR.print(" and calculated rpm is ");
  // SERIAL_PORT_MONITOR.println(currentRpm);

  return currentRpm;
}

/****************************************************
 *
 * Function - Calculate and set radiator fan output
 *
 ****************************************************/
const float fanMinimumEngineTemperature = 90;  // Temperature in celcius when fan will begin opperation
const float fanMaximumEngineTemperature = 95; // Temperature in celcius when fan will be opperating at maximum power
float fanPercentageOutput = 0.0;               // Will store the current fan output percentage
int fanMinimumPercentageOutput = 40;           // A reasonable minimum fan speed to avoid running it too slow
int fanPwmPinValue = 0; // Will store the PWM pin value from 0 - 255 to interface with the motor driver board

int setRadiatorFanOutput(int engineTemp, int engineRpm, byte signalPin) {
  if (engineTemp >= fanMaximumEngineTemperature) {
    fanPercentageOutput = 100;
  } else if (engineTemp > fanMinimumEngineTemperature) {
    int degreesAboveMinimum = engineTemp - fanMinimumEngineTemperature;
    fanPercentageOutput = (degreesAboveMinimum / (fanMaximumEngineTemperature - fanMinimumEngineTemperature)) * 100;
    if (fanPercentageOutput < fanMinimumPercentageOutput) {
      fanPercentageOutput = fanMinimumPercentageOutput;
    }
  } else {
    fanPercentageOutput = 0;
  }

  // Here we will actually set the external PWM control based on calculated percentage output, but only if the engine is
  // running
  if (fanPercentageOutput != 0 && engineRpm > 500) {
    fanPwmPinValue = fanPercentageOutput * 2.55;
  } else {
    fanPwmPinValue = 0;
  }

  // Write out the actual pin value, the pin will maintain this output until the next update
  analogWrite(signalPin, fanPwmPinValue);

  return fanPercentageOutput;
}

/****************************************************
 *
 * Function - Sound the alarm (but only after a delay) !!
 *
 ****************************************************/
unsigned long inAlarmDuration = 0;
unsigned long firstAlarmCallTime = 0;

void alarmEnable(int alarmBuzzerPin, int engineRpm) {
  if (firstAlarmCallTime == 0) {
    firstAlarmCallTime = millis();
  }

  inAlarmDuration = millis() - firstAlarmCallTime;

  if (engineRpm > 500 && inAlarmDuration > 1000) {
    tone(alarmBuzzerPin, 4000);
  } else if (engineRpm == 0) {
    alarmDisable(alarmBuzzerPin);
  }
}

/****************************************************
 *
 * Function - Disable the alarm please thanks
 *
 ****************************************************/
void alarmDisable(int alarmBuzzerPin) { 
  noTone(alarmBuzzerPin);
  firstAlarmCallTime = 0;
}
