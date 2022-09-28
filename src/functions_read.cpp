#include "functions_read.h"
#include <Adafruit_MCP9808.h>   // Used for temperature sensor

/**
 *
 * Function - Get the current engine bay electronics temp
 *
 */

float readEngineElectronicsTemp(Adafruit_MCP9808 t) {
    return t.readTempC();
}
