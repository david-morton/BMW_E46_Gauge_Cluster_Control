#include "functions_performance.h"
#include <Arduino.h>

/*****************************************************
 *
 * Function - Detect interesting speeds and calculate accelleration times to display
 *
 ****************************************************/

float latest0To50 = 300000;
float best0To50 = 300000;

float latest0To100 = 300000;
float best0To100 = 300000;

float latest80To120 = 300000;
float best80To120 = 300000;

unsigned long start0To50;
unsigned long end0To50;

unsigned long start0To100;
unsigned long end0To100;

unsigned long start80To120;
unsigned long end80To120;

float previousSpeed;

float getBestZeroToFifty() { return best0To50; }
float getBestZeroToOneHundred() { return best0To100; }
float getBestEightyToOneTwenty() { return best80To120; }

void captureAccellerationTimes(unsigned long speedTimestamp, float speedValue) {
  if (speedValue < 1) {
    speedValue = 0;
  }

  // Capture start condition 0 to 50
  if (speedValue > 0 && previousSpeed == 0) {
    start0To50 = speedTimestamp;
  }

  // Capture end condition 0 to 50
  if (speedValue >= 50 && previousSpeed < 50) {
    end0To50 = speedTimestamp;
    latest0To50 = end0To50 - start0To50;
    if (latest0To50 < best0To50) {
      best0To50 = latest0To50;
    }
  }

  // Capture start condition 0 to 100
  if (speedValue > 0 && previousSpeed == 0) {
    start0To100 = speedTimestamp;
  }

  // Capture end condition 0 to 100
  if (speedValue >= 100 && previousSpeed < 100) {
    end0To100 = speedTimestamp;
    latest0To100 = end0To100 - start0To100;
    if (latest0To100 < best0To100) {
      best0To100 = latest0To100;
    }
  }

  // Capture start condition 80 to 120
  if (speedValue >= 80 && previousSpeed < 80) {
    start80To120 = speedTimestamp;
  }

  // Capture end condition 80 to 120
  if (speedValue >= 120 && previousSpeed < 120) {
    end80To120 = speedTimestamp;
    latest80To120 = end80To120 - start80To120;
    if (latest80To120 < best80To120) {
      best80To120 = latest80To120;
    }
  }

  // Set the previous speed so we can use it for triggers on the next execution
  previousSpeed = speedValue;
}

/*****************************************************
 *
 * Function - Capture and plot fine grained speed data to make dyno like comparisons
 *
 ****************************************************/

// Define an array to hold fine grained performance metric data, this will allow plotting a bit like a dyno chart
// A larger array will allow a wider speed delta for dyno runs but we are memory bound here
const int maxArraySamples = 600;
performanceData performanceDataArray[maxArraySamples];
int arrayLocation = 0;
int sampleResolutionMs = 20; // Record the sample data only every n milliseconds as we don't have much memory

// Define other self explanatory variables
const float dynoStartSpeed = 25;
const float dynoEndSpeed = 100;

unsigned long dynoStartTimestamp;
unsigned long dynoMillisElapsed;
unsigned long dynoMillisCutofftime = 10000;

float previousDynoSpeed;
unsigned long previousSpeedTimestamp;

bool dynoRunActive = false;

void captureAccellerationDetailedData(unsigned long speedTimestamp, float speedValue) {
  // Determine if we are entering a potential dyno run
  if (speedValue >= dynoStartSpeed && previousDynoSpeed < dynoStartSpeed && dynoRunActive == false) {
    dynoRunActive = true;
    dynoStartTimestamp = speedTimestamp;
    Serial.println("Starting dyno run data logging ...");
  }

  // Log dyno data if we are in an active run and conditions are met, tap out if too many samples recorded
  if (dynoRunActive == true && speedValue != previousDynoSpeed &&
      speedTimestamp > (previousSpeedTimestamp + sampleResolutionMs)) {
    dynoMillisElapsed = speedTimestamp - dynoStartTimestamp;
    performanceDataArray[arrayLocation].timestamp = dynoMillisElapsed;
    performanceDataArray[arrayLocation].speed = speedValue;
    previousSpeedTimestamp = speedTimestamp;
    arrayLocation++;
    // Stop recording if we reach max samples
    if (arrayLocation == (maxArraySamples - 1)) {
      Serial.print("Stopping dyno run due to max samples reached.");
      memset(performanceDataArray, 0, sizeof(performanceDataArray));
      arrayLocation = 0;
      dynoRunActive = false;
    }
  }

  // Exit the dyno run if we detect a complete run
  if (dynoRunActive == true && speedValue >= dynoEndSpeed && previousDynoSpeed < dynoEndSpeed) {
    dynoRunActive = false;
    Serial.print("Stopping dyno run data log and clearing array after ");
    Serial.print(arrayLocation + 1);
    Serial.println(" samples.");
    // Print the output for later diagnosis / graphing
    for (int i = 0; i < arrayLocation; i++) {
      Serial.print(performanceDataArray[i].timestamp);
      Serial.print(",");
      Serial.println(performanceDataArray[i].speed);
    }
    memset(performanceDataArray, 0, sizeof(performanceDataArray));
    arrayLocation = 0;
    Serial.println("");
  }

  // Exit the dyno run if we time out
  if (dynoRunActive == true && dynoMillisElapsed > dynoMillisCutofftime) {
    dynoRunActive = false;
    Serial.println("Stopping dyno run due to timeout");
    Serial.print("dynoMilliselapsed: ");
    Serial.print(dynoMillisElapsed);
    Serial.print(" > ");
    Serial.println(dynoMillisCutofftime);
    Serial.println("");
    memset(performanceDataArray, 0, sizeof(performanceDataArray));
    arrayLocation = 0;
  }
  previousDynoSpeed = speedValue;
}
