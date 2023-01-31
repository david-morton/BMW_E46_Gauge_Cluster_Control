#ifndef FUNCTIONS_PERFORMANCE_H
#define FUNCTIONS_PERFORMANCE_H

/****************************************************
 *
 * Custom Data Types
 *
 ****************************************************/
struct performanceData {
  int timestamp;
  float speed;
};

/****************************************************
 *
 * Function Prototypes
 *
 ****************************************************/

void captureAccellerationTimes(unsigned long timestamp, float speed);
void captureAccellerationDetailedData(unsigned long timestamp, float speed);
float getBestZeroToOneHundred();
float getBestEightyToOneTwenty();

#endif
