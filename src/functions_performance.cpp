#include "functions_performance.h"
#include <Arduino.h>

/*****************************************************
 *
 * Function - Detect interesting speeds and calculate stuff
 * 
 ****************************************************/

float latest0to50 = 300000;
float best0to50   = 300000;

bool measuring0To50 = false;
unsigned long start0To50;
unsigned long end0To50;

float getBestZeroToFifty(){
    return best0to50;
}

void captureAccellerationTimes(unsigned long timestamp, float speed){
    if (speed < 1) {
        speed = 0;
    }

    // General debug for update frequency etc
    SERIAL_PORT_MONITOR.print(timestamp);
    SERIAL_PORT_MONITOR.print(",");
    SERIAL_PORT_MONITOR.println(speed);

    // Capture 0-50 speed
    if (speed == 0) {
        SERIAL_PORT_MONITOR.println("Setting measurement flag to true");
        measuring0To50 = true;
        start0To50 = timestamp;
    }
    if (measuring0To50 == true && speed >= 50) {
        measuring0To50 = false;
        end0To50 = timestamp;
        latest0to50 = end0To50 - start0To50;
        SERIAL_PORT_MONITOR.print("Detected new 0-50 capture of: ");
        SERIAL_PORT_MONITOR.println(latest0to50);
    }
    if (latest0to50 < best0to50) {
        best0to50 = latest0to50;
        SERIAL_PORT_MONITOR.print("New 0-50 time recorded: ");
        SERIAL_PORT_MONITOR.println(best0to50);
    }
}
