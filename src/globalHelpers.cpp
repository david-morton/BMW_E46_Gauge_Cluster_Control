#include "globalHelpers.h"
#include <light_CD74HC4067.h>

/* ======================================================================
   VARIABLES: Pin constants
   ====================================================================== */
// Needed for multiplexer board setup
const int muxS0Pin = 7;
const int muxS1Pin = 8;
const int muxS2Pin = 15;
const int muxS3Pin = 16;
const byte muxSignalPin = A0;

/* ======================================================================
   OBJECT DECLARATIOS
   ====================================================================== */
CD74HC4067 mux(muxS0Pin, muxS1Pin, muxS2Pin, muxS3Pin);

/* ======================================================================
   VARIABLES: General use / functional
   ====================================================================== */
const int millisWithoutSerialCommsBeforeFault = 1000; // How long is ms without serial comms from the master before we declare a critical alarm

/* ======================================================================
   FUNCTION: Setup the multiplexer
   ====================================================================== */
void setupMux() {
  DEBUG_GENERAL("Configuring mux board ...");
  pinMode(muxSignalPin, INPUT);
}

/* ======================================================================
   FUNCTION: Get averaged analogue reading from analogue pin
   ====================================================================== */
int getAveragedAnaloguePinReading(byte pin, int samples, int delayUs) {
  int totalReadings = 0;

  for (int i = 0; i < samples; i++) {
    if (delayUs != 0) {
      delayMicroseconds(delayUs);
    }
    totalReadings += analogRead(pin);
  }

  // Calculate average of the readings
  int averageReading = totalReadings / static_cast<float>(samples);
  return averageReading;
}

/* ======================================================================
   FUNCTION: Get averaged analogue reading from multiplexed channel via CD74HC4067
   ====================================================================== */
int getAveragedMuxAnalogueChannelReading(byte channel, int samples, int delayUs) {
  mux.channel(channel);
  int averageReading = getAveragedAnaloguePinReading(muxSignalPin, samples, delayUs);
  return averageReading;
}

/* ======================================================================
   FUNCTION: Get digital value from multiplexed channel via CD74HC4067
   ====================================================================== */
bool getMuxDigitalChannelValue(byte channel) {
  mux.channel(channel);
  return digitalRead(muxSignalPin);
}

/* ======================================================================
   FUNCTION: Report Arduino loop rate information
   ====================================================================== */
unsigned long arduinoLoopExecutionPreviousExecutionMillis;

void reportArduinoLoopRate(unsigned long *loopCount) {
  float loopFrequencyHz = (*loopCount / ((millis() - arduinoLoopExecutionPreviousExecutionMillis) / 1000));
  float loopExecutionMs = (millis() - arduinoLoopExecutionPreviousExecutionMillis) / *loopCount;
  Serial.print("Loop execution frequency (Hz): ");
  Serial.print(loopFrequencyHz);
  Serial.print(" or every ");
  Serial.print(loopExecutionMs);
  Serial.println("ms");
  *loopCount = 1;
  arduinoLoopExecutionPreviousExecutionMillis = millis();
}
