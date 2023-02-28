#ifndef FUNCTIONS_DO_H
#define FUNCTIONS_DO_H

#include <Arduino.h>

/****************************************************
 *
 * Function Prototypes
 *
 ****************************************************/
int calculateRpm();
void updateRpmPulse();
int setRadiatorFanOutput(int, int, byte);
void alarmEnable(int, int);
void alarmDisable(int);

#endif
