#include "gearCalculation.h"
#include <Arduino.h>

/* ======================================================================
   FUNCTION: Determine if we are in neutral
   ====================================================================== */
bool getNeutralStatus() {
  return true;
}

/* ======================================================================
   FUNCTION: Determine if the clutch is pressed
   ====================================================================== */
bool getClutchStatus() {
  return true;
}

/* ======================================================================
   FUNCTION: Determine which gear we are in
   ====================================================================== */
const int tyreWidth = 255;
const int tyreProfile = 35;
const int wheelSizeInches = 18;
const int rollingCircumferenceMm = PI * ((wheelSizeInches * 25.4) + (2 * (tyreWidth * tyreProfile / 100.0)));

const float ratioFinalDrive = 3.38;
const float ratioGear1 = 3.794;
const float ratioGear2 = 2.324;
const float ratioGear3 = 1.624;
const float ratioGear4 = 1.271;
const float ratioGear5 = 1.0;
const float ratioGear6 = 0.794;

int getCurrentGear(int *rpm, float *rearWheelSpeed, bool clutchPressed, bool inNeutral) {
  if (clutchPressed == true || inNeutral == true || rearWheelSpeed == 0) {
    return 0;
  } else {
    // Calculate rear wheel speed in revolutions per minute
    float rearWheelRpm = ((*rearWheelSpeed * 1000 / 60) * 1000) / rollingCircumferenceMm;

    // Calculate driveshaft speed considering final drive ratio
    float driveShaftRpm = rearWheelRpm * ratioFinalDrive;

    // Calculate observed ratio per gear and return when within error limits
  }
}

// 1986mm circumference
// for 50kph
// 420 rpm at the wheel
// 1420 rpm at the driveshaft

// 1850rpm as test case (we should determind 4th gear)

// 1st = 5387
// 2nd = 3300
// 3rd = 2306
// 4th = 1804
// 5th = 1420
// 6th = 1127
