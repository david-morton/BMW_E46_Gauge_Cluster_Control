#ifndef GLOBALHELPERS_H
#define GLOBALHELPERS_H

#include <Arduino.h>

/* ======================================================================
   HELPER: Global flags and functions for debugging
   ====================================================================== */
extern bool debugSerialReceive;
extern bool debugSerialSend;
extern bool debugGeneral;

/* ======================================================================
   HELPERS: Debug output definitions
   ====================================================================== */
// Define the DEBUG_SERIAL_RECEIVE macro
#define DEBUG_SERIAL_RECEIVE(message)          \
  do {                                         \
    if (debugSerialReceive) {                  \
      Serial.print("[DEBUG SERIAL RECEIVE] "); \
      Serial.println(message);                 \
    }                                          \
  } while (0)

// Define the DEBUG_SERIAL_SEND macro
#define DEBUG_SERIAL_SEND(message)          \
  do {                                      \
    if (debugSerialSend) {                  \
      Serial.print("[DEBUG SERIAL SEND] "); \
      Serial.println(message);              \
    }                                       \
  } while (0)

// Define the DEBUG_GENERAL macro
#define DEBUG_GENERAL(message)          \
  do {                                  \
    if (debugGeneral) {                 \
      Serial.print("[DEBUG GENERAL] "); \
      Serial.println(message);          \
    }                                   \
  } while (0)

/* ======================================================================
   FUNCTION PROTOTYPES
   ====================================================================== */
int getAveragedAnaloguePinReading(byte, int, int);
int getAveragedMuxAnalogueChannelReading(byte, int, int);
void reportArduinoLoopRate(unsigned long *);
void setupMux();

#endif
