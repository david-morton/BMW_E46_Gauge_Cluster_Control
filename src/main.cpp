/* This is a basic program to drive the E46 gauge cluster via CAN. The hardware used
was a single Arduino Mega 2560 r3 and two Seeed CAN-Bus shield v2's.

This project was used in the context of an engine swap where a Nissan VQ37VHR was
installed into the E46, and the desire was to maintain a functional OEM dashboard.

The scaling ratio of engine RPM to the hex value of CAN payload is 
approximated in this program based on a linear formula worked out in Excel.

This was done from a test cluster so that visually the needle lined up
as well as possible throughout the RPM range. Individual clusters may vary
so some experimentation with this may be desired ... practically speaking it
may make no differance at all :)

For reference the measured ratios at varios RPM's in my own test cluster are as per the below:
1000 6.65
2000 6.59
3000 6.53
4000 6.48
5000 6.42
6000 6.37
7000 6.31

The fuel economy gauge only works when there is a speed input. Inputting a square wave of
410Hz and 50% duty cycle gives 60km/h. This signal is put into pin 19 on cluster connector
X11175 for testing purposes on the bench. This signal is usually provided by the ABS computer.

The lowest speed pulse generation that seems to allow activation of the fuel economy gauge is
82Hz when increasing and it will stop function at 68Hz when decreasing. 82Hz seems to be about
5kph.

VQ37 ECU Pin 110 is 'Engine speed output signal' and outputs a square wave at 3 pulses per
revolution. We use this signal to calculate the engine RPM and send the value to the cluster.

The Arduino will also be used to control the radiator thermo fan via a Cytron MD30C. This
is a PWM motor controller suitable for brushed DC motors up to a constant 30A.
*/

#include <SPI.h>
#include <Wire.h>
#include <mcp2515_can.h>        // Used for Seeed shields
#include <Adafruit_MCP9808.h>   // Used for temperature sensor
#include <ptScheduler.h>
#include "functions_read.h"
#include "functions_write.h"
#include "functions_do.h"

#define CAN_2515

// Pin assignments all go here
const int SPI_SS_PIN_BMW = 9;              // Slave select pin for CAN shield 1 (BMW CAN bus)
const int SPI_SS_PIN_NISSAN = 10;          // Slave select pin for CAN shield 2 (Nissan CAN bus)
const int CAN_INT_PIN = 2;
const int ENGINE_CHECK_LED = 40;           // Used for an external LED in case we don't trust the dash check light

const byte rpmSignalPin = 19;              // Digital input pin for signal wire and interrupt (from Nissan ECU)
const byte fanDriverPwmSignalPin = 44;     // Digital output pin for PWM signal to radiator fan motor driver board

// Define CAN objects
mcp2515_can CAN_BMW(SPI_SS_PIN_BMW);
mcp2515_can CAN_NISSAN(SPI_SS_PIN_NISSAN);

// Define variables used specifically for tachometer
int currentRpm;                         // Will store the current engine RPM value

// Define variables used for radiator fan control
int currentEngineTempCelsius;
int currentCheckEngineLightState;

// Define other variables
const int tempAlarmLight = 110;         // What temperature should the warning light come on at
float currentEngineElectronicsTemp;     // Will store the temperature in celcius of the engine bay electronics
int consumptionCounter = 0;
int consumptionIncrease = 40;
int consumptionValue = 0;
int setupRetriesMax = 3;                // The number of times we should loop with delay to configure shields etc
int currentVehicleSpeed = 0;

// Define CAN payloads
unsigned char canPayloadMisc[8] = {0, 0, 0, 0, 0, 0, 0, 0};    //Misc (check light, consumption and temp alarm light)

// Create the MCP9808 temperature sensor object
Adafruit_MCP9808 tempSensorEngineElectronics = Adafruit_MCP9808();

// Function - Write misc payload to BMW CAN
void canWriteMisc() {
    canPayloadMisc[0] = currentCheckEngineLightState;   // 2 for check engine light
                                                        // 16 for EML light
                                                        // 18 for check engine AND EML
                                                        // 0 for neither
    canPayloadMisc[1] = consumptionValue;               // Fuel consumption LSB                                        
    canPayloadMisc[2] = (consumptionValue >> 8);        // Fuel consumption MSB
    if (currentEngineTempCelsius >= tempAlarmLight)     // Set the red alarm light on the temp gauge if needed
        canPayloadMisc[3] = 8;
    else
        canPayloadMisc[3] = 0;

    if (consumptionCounter % 1 == 0) {
        consumptionValue += consumptionIncrease;
    }

    consumptionCounter++;

    CAN_BMW.sendMsgBuf(0x545, 0, 8, canPayloadMisc);
}

// Define our pretty tiny scheduler objects
ptScheduler ptCalculateRpm              = ptScheduler(PT_TIME_20MS);
ptScheduler ptCanWriteRpm               = ptScheduler(PT_TIME_10MS);
ptScheduler ptCanWriteTemp              = ptScheduler(PT_TIME_10MS);
ptScheduler ptCanWriteSpeed             = ptScheduler(PT_TIME_20MS);
ptScheduler ptCanWriteMisc              = ptScheduler(PT_TIME_10MS);
ptScheduler ptSetRadiatorFanOutput      = ptScheduler(PT_TIME_5S);
ptScheduler ptReadEngineElectronicsTemp = ptScheduler(PT_TIME_5S);

// Our main setup stanza
void setup() {
    SERIAL_PORT_MONITOR.begin(115200);
    while(!Serial){};

    // Configure CAN shield interfaces
    bool canBmwFound;
    bool canNissanFound;

    SERIAL_PORT_MONITOR.println("INFO: Initialising BMW CAN shield");

    for (int i = 0; i < setupRetriesMax; i++) {
        bool result = CAN_BMW.begin(CAN_500KBPS);
        if (result == CAN_OK) {
            SERIAL_PORT_MONITOR.println("\tOK - BMW CAN shield initialised");
            canBmwFound = true;
            break;
        } else if (result != CAN_OK) {
            SERIAL_PORT_MONITOR.println("\tERROR - BMW CAN shield init failed, retrying ...");
            canBmwFound = false;
            delay(1000);
        }
        if (i == setupRetriesMax)
        {
            SERIAL_PORT_MONITOR.println("\tFATAL - BMW CAN shield init failed");
        }
    }
    
    SERIAL_PORT_MONITOR.println("INFO: Initialising Nissan CAN shield");

    for (int i = 0; i < setupRetriesMax; i++) {
        bool result = CAN_NISSAN.begin(CAN_500KBPS);
        if (result == CAN_OK) {
            SERIAL_PORT_MONITOR.println("\tOK - Nissan CAN shield initialised");
            canNissanFound = true;
            break;
        } else if (result != CAN_OK) {
            SERIAL_PORT_MONITOR.println("\tERROR - Nissan CAN shield init failed, retrying ...");
            canNissanFound = false;
            delay(1000);
        }
        if (i == setupRetriesMax)
        {
            SERIAL_PORT_MONITOR.println("\tFATAL - Nissan CAN shield init failed");
        }
    }

    // Configure interrupt for RPM signal input
    pinMode(rpmSignalPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(rpmSignalPin), updateRpmPulse, RISING);

    // Configure pins for output to fan controller
    pinMode(fanDriverPwmSignalPin, OUTPUT);
        
    // Configure the temperature sensor
    bool tempSensorFound;

    // Configure the pin which drives the check engine LED
    pinMode(ENGINE_CHECK_LED, OUTPUT);

    SERIAL_PORT_MONITOR.println("INFO: Initialising MCP9808 temperature sensor");

    for (int i = 0; i < setupRetriesMax; i++) {
        bool result = tempSensorEngineElectronics.begin(0x18);
        if (result == 0) {
            SERIAL_PORT_MONITOR.println("\tOK - MCP9808 temperature sensor initialised");
            tempSensorFound = true;
            tempSensorEngineElectronics.readTempC();
            break;
        } else if (result != 0) {
            SERIAL_PORT_MONITOR.println("\tERROR - MCP9808 temperature sensor init failed, retrying ...");
            tempSensorFound = false;
            delay(1000);
        }
        if (i == setupRetriesMax)
        {
            SERIAL_PORT_MONITOR.println("\tFATAL - MCP9808 temperature sensor not found");
        }
    }

    // Configure masks and filters for shields to reduce noise
    // There are two masks in the mcp2515 which both need to be set
    // Mask 0 has 2 filters and mask 1 has 4 so we set them all as needed
    // 0x551 is where coolant temperature is located
    // 0x180 is where the check engine light is
    CAN_NISSAN.init_Mask(0, 0, 0xFFF);
    CAN_NISSAN.init_Filt(0, 0, 0x551);
    CAN_NISSAN.init_Filt(1, 0, 0x180);

    CAN_NISSAN.init_Mask(1, 0, 0xFFF);
    CAN_NISSAN.init_Filt(2, 0, 0x551);
    CAN_NISSAN.init_Filt(3, 0, 0x180);
    CAN_NISSAN.init_Filt(4, 0, 0x551);
    CAN_NISSAN.init_Filt(5, 0, 0x180);

    // 0x1F0 is where the individual wheel speeds are
    // https://www.bimmerforums.com/forum/showthread.php?1887229-E46-Can-bus-project
    CAN_BMW.init_Mask(0, 0, 0xFFF);
    CAN_BMW.init_Filt(0, 0, 0x1F0);
    CAN_BMW.init_Filt(1, 0, 0x1F0);

    CAN_BMW.init_Mask(1, 0, 0xFFF);
    CAN_BMW.init_Filt(2, 0, 0x1F0);
    CAN_BMW.init_Filt(3, 0, 0x1F0);
    CAN_BMW.init_Filt(4, 0, 0x1F0);
    CAN_BMW.init_Filt(5, 0, 0x1F0);
}

// Our main loop
void loop() {
    // Check the pretty tiny scheduler tasks and call as needed
    if (ptCalculateRpm.call()) {
        currentRpm = calculateRpm();
    }

    if (ptCanWriteRpm.call()) {
        canWriteRpm(currentRpm, CAN_BMW);
    }

    if (ptCanWriteTemp.call()) {
        canWriteTemp(currentEngineTempCelsius, CAN_BMW);
    }

     if (ptCanWriteSpeed.call()) {
        canWriteSpeed(currentVehicleSpeed, CAN_NISSAN);
    }

    if (ptCanWriteMisc.call()) {
        canWriteMisc();
    }

    if (ptSetRadiatorFanOutput.call()) {
        setRadiatorFanOutput(currentEngineTempCelsius, currentRpm, fanDriverPwmSignalPin);
    }

    if (ptReadEngineElectronicsTemp.call()) {
        currentEngineElectronicsTemp = readEngineElectronicsTemp(tempSensorEngineElectronics);
    }

    // Fetch the latest values from Nissan CAN
    nissanCanValues currentNissanCanValues = readNissanDataFromCan(CAN_NISSAN);

    // Fetch the latest values from BMW CAN
    bmwCanValues currentBmwCanValues = readBmwDataFromCan(CAN_BMW);

    // Pull the values were are interested in from the Nissan CAN response
    currentEngineTempCelsius = currentNissanCanValues.engineTempCelsius;
    currentCheckEngineLightState = currentNissanCanValues.checkEngineLightState;

    // Pull the values were are interested in from the BMW CAN response
    currentVehicleSpeed = currentBmwCanValues.vehicleSpeed;
    SERIAL_PORT_MONITOR.println(currentVehicleSpeed);

    // Light the external LED check light
    if ( currentCheckEngineLightState == 2 )
    {
        digitalWrite(ENGINE_CHECK_LED, HIGH);
    } 
    else
    {
        digitalWrite(ENGINE_CHECK_LED, LOW);
    }    
}
