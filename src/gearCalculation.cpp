#include "gearCalculation.h"
#include "globalHelpers.h"
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

int currentGear = 0;
int previousGear = 0;

// Define a custom structure to hold my result deltas
struct DriveShaftRpmDelta {
  int gearNumber;
  float rpmDelta;
};

// Define an array of all defined ratios
const float gearRatios[] = {ratioGear1, ratioGear2, ratioGear3, ratioGear4, ratioGear5, ratioGear6};

// Calculate the number of gear ratio elements in the array
const int numberOfGears = sizeof(gearRatios) / sizeof(gearRatios[0]);

// Create array to hold results
DriveShaftRpmDelta driveShaftRpmDeltas[numberOfGears];

// Define the function itself
int getCurrentGear(int *rpm, float *rearWheelSpeed, bool clutchPressed, bool inNeutral) {
  if (clutchPressed == true || inNeutral == true || rearWheelSpeed == 0) {
    return 0;
  } else {
    // Calculate rear wheel speed in revolutions per minute
    float rearWheelRpm = ((*rearWheelSpeed * 1000 / 60) * 1000) / rollingCircumferenceMm;

    // Calculate driveshaft speed considering final drive ratio
    float actualDriveShaftRpm = rearWheelRpm * ratioFinalDrive;

    // Calculate delta between calculated and actual driveshaft rpm's and store results in array
    for (int i = 0; i < numberOfGears; i++) {
      float calculatedDriveShaftRpm = *rpm / gearRatios[i];
      float driveShaftRpmDelta = abs(actualDriveShaftRpm - calculatedDriveShaftRpm);
      driveShaftRpmDeltas[i].gearNumber = i + 1;
      driveShaftRpmDeltas[i].rpmDelta = driveShaftRpmDelta;
    }

    // Decide which is the closest ratio match
    int smallestIndex = 0;
    float smallestdriveShaftRpmDelta = driveShaftRpmDeltas[0].rpmDelta;

    for (int i = 1; i < numberOfGears; ++i) {
      if (driveShaftRpmDeltas[i].rpmDelta < smallestdriveShaftRpmDelta) {
        smallestIndex = i;
        smallestdriveShaftRpmDelta = driveShaftRpmDeltas[i].rpmDelta;
      }
    }

    // Return the gear with the closest ratio match
    currentGear = driveShaftRpmDeltas[smallestIndex].gearNumber;

    // Add some debug output when a gear change is detected
    if (currentGear != previousGear) {
      DEBUG_GEARS("Gear change detected from " + String(previousGear) + " to " + String(currentGear) + " with a ratio delta of " + String(driveShaftRpmDeltas[smallestIndex].rpmDelta));
      previousGear = currentGear;
    }
    return currentGear;
  }
}
