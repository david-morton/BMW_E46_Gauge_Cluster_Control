#ifndef GEARCALCULATION_H
#define GEARCALCULATION_H

#include <Arduino.h>

/* ======================================================================
   FUNCTION PROTOTYPES
   ====================================================================== */
int getCurrentGear(int *, float *, bool *, bool *);
bool getClutchStatus(byte);
bool getNeutralStatus(byte);

#endif
