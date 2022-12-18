#include "functions_performance.h"
#include <Arduino.h>

/*****************************************************
 *
 * Function - Detect interesting speeds and calculate stuff
 *
 ****************************************************/

float latest0To50 = 300000;
float best0To50 = 300000;

float latest80To120 = 300000;
float best80To120 = 300000;

// bool measuring0To50 = false;
unsigned long start0To50;
unsigned long end0To50;

unsigned long start80To120;
unsigned long end80To120;

float previousSpeed;

float getBestZeroToFifty() { return best0To50; }
float getBestEightyToOneTwenty() { return best80To120; }

void captureAccellerationTimes(unsigned long speedTimestamp, float speedValue) {
  if (speedValue < 1) {
    speedValue = 0;
  }

  // // Capture start condition 0 to 50
  // if (speedValue == 0) {
  //   measuring0To50 = true;
  //   start0To50 = speedTimestamp;
  // }

  // // Capture end condition 0 to 50
  // if (measuring0To50 == true && speedValue >= 50) {
  //   measuring0To50 = false;
  //   end0To50 = speedTimestamp;
  //   latest0To50 = end0To50 - start0To50;
  //   if (latest0To50 < best0To50) {
  //     best0To50 = latest0To50;
  //   }
  // }

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
