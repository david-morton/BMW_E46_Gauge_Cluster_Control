#include "functions_do.h"

/**
 *
 * ISR - Update the RPM counter and time via interrupt
 *
 */
volatile unsigned long latestRpmPulseTime = micros();   // Will store latest ISR micros value for calculations
volatile unsigned long latestRpmPulseCounter = 0;       // Will store latest the number of pulses counted

void updateRpmPulse() {
    latestRpmPulseCounter ++;
    latestRpmPulseTime = micros();
}

/**
 *
 * Function - Calculate the current RPM value from pulses measured via ISR
 *
 */
const int rpmPulsesPerRevolution = 3;                   // Number of pulses on the signal wire per crank revolution
unsigned long previousRpmPulseTime;                     // Will store previous ISR micros value for calculations
unsigned long previousRpmPulseCounter;                  // Will store previous the number of pulses counted

int calculateRpm(){
    unsigned long deltaMicros = latestRpmPulseTime - previousRpmPulseTime;
    unsigned long deltaRpmPulseCounter = latestRpmPulseCounter - previousRpmPulseCounter;

    float microsPerPulse = deltaMicros / deltaRpmPulseCounter;

    float pulsesPerMinute = 60000000 / microsPerPulse;

    // Set previous variables to current values for next loop
    previousRpmPulseCounter = latestRpmPulseCounter;
    previousRpmPulseTime = latestRpmPulseTime;

    return pulsesPerMinute / 3;
}