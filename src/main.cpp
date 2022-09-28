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

#define CAN_2515

// Pin assignments all go here
const int SPI_SS_PIN_BMW = 9;              // Slave select pin for CAN shield 1 (BMW CAN bus)
const int SPI_SS_PIN_NISSAN = 10;          // Slave select pin for CAN shield 2 (Nissan CAN bus)
const int CAN_INT_PIN = 2;
const byte rpmSignalPin = 19;              // Digital input pin for signal wire and interrupt (from Nissan ECU)
const byte fanDriverPwmSignalPin = 44;     // Digital output pin for PWM signal to radiator fan motor driver board
const byte fanDriverPwmDirectionPin = 43;  // Digital output pin for PWM direction for radiator fan motor driver board

mcp2515_can CAN_BMW(SPI_SS_PIN_BMW);
mcp2515_can CAN_NISSAN(SPI_SS_PIN_NISSAN);

// Define variables used specifically for RPM and tachometer
const int rpmPulsesPerRevolution = 3;                   // Number of pulses on the signal wire per crank revolution
volatile unsigned long latestRpmPulseTime = micros();   // Will store latest ISR micros value for calculations
volatile unsigned long latestRpmPulseCounter = 0;       // Will store latest the number of pulses counted
unsigned long previousRpmPulseTime;                     // Will store previous ISR micros value for calculations
unsigned long previousRpmPulseCounter;                  // Will store previous the number of pulses counted
int currentRpm;                                         // Will store the current RPM value                                                 // This will be overriden via the formula based multiplier later on if used.

// Define variables used for radiator fan control
const float fanMinimumEngineTemperature = 90;           // Temperature in celcius when fan will begin opperation
const float fanMaximumEngineTemperature = 105;          // Temperature in celcius when fan will be opperating at maximum power
float fanPercentageOutput = 0.0;                        // Will store the current fan output percentage
int fanPwmPinValue = 0;                                 // Will store the PWM pin value from 0 - 255 to interface with the motor driver board

// Define other variables
const int tempAlarmLight = 110;                         // What temperature should the warning light come on at
int currentEngineTempCelsius;
int checkEngineLightState;
unsigned long engineCheckTriggeredMillis = 1;           // Holds the timestamp when engine check was triggered
const int minimumEngineCheckLightDuration = 3;          // How many seconds should engine check light display for minimum
                                                        // This is to ensure it illuminates at ignition on as it is too fast
                                                        // to show on the BMW cluster. This is only useful if you power the
                                                        // Arduino via accessory power, in my case the check light happens
                                                        // before the Arduino boots up and can push the CAN payload.
float currentEngineElectronicsTemp;                     // Will store the temperature in celcius of the engine bay electronics
int consumptionCounter = 0;
int consumptionIncrease = 40;
int consumptionValue = 0;

// Define CAN payloads
unsigned char canPayloadMisc[8] = {0, 0, 0, 0, 0, 0, 0, 0};    //Misc (check light, consumption and temp alarm light)

// Create the MCP9808 temperature sensor object
Adafruit_MCP9808 tempSensorEngineElectronics = Adafruit_MCP9808();

// Function - Calculate current RPM
void calculateRpm(){
    unsigned long deltaMicros = latestRpmPulseTime - previousRpmPulseTime;
    unsigned long deltaRpmPulseCounter = latestRpmPulseCounter - previousRpmPulseCounter;

    float microsPerPulse = deltaMicros / deltaRpmPulseCounter;

    float pulsesPerMinute = 60000000 / microsPerPulse;

    currentRpm = pulsesPerMinute / 3;

    // Set previous variables to current values for next loop
    previousRpmPulseCounter = latestRpmPulseCounter;
    previousRpmPulseTime = latestRpmPulseTime;
}

// Function - Write misc payload to BMW CAN
void canWriteMisc() {
    canPayloadMisc[0] = checkEngineLightState;      // 2 for check engine light
                                                    // 16 for EML light
                                                    // 18 for check engine AND EML
                                                    // 0 for neither
    canPayloadMisc[1] = consumptionValue;           // Fuel consumption LSB                                        
    canPayloadMisc[2] = (consumptionValue >> 8);    // Fuel consumption MSB
    if (currentEngineTempCelsius >= tempAlarmLight) // Set the red alarm light on the temp gauge if needed
        canPayloadMisc[3] = 8;
    else
        canPayloadMisc[3] = 0;

    if (consumptionCounter % 1 == 0) {
        consumptionValue += consumptionIncrease;
    }

    consumptionCounter++;

    CAN_BMW.sendMsgBuf(0x545, 0, 8, canPayloadMisc);
}

// ISR - Update the RPM counter and time via interrupt
void updateRpmPulse() {
    latestRpmPulseCounter ++;
    latestRpmPulseTime = micros();
}

// Function - Print temp reading to serial monitor
void outputDebugInfo() {
    SERIAL_PORT_MONITOR.print("Temperature is: ");
    SERIAL_PORT_MONITOR.print(currentEngineTempCelsius);
    SERIAL_PORT_MONITOR.print("\tFan Percentage is: ");
    SERIAL_PORT_MONITOR.print(fanPercentageOutput);
    SERIAL_PORT_MONITOR.print("\tFan pin value is: ");
    SERIAL_PORT_MONITOR.println(fanPwmPinValue);
    SERIAL_PORT_MONITOR.print("\tEngine bay electronics temp is: ");
    SERIAL_PORT_MONITOR.println(currentEngineElectronicsTemp);
}

// Function - Read latest values from Nissan CAN
void updateNissanDataFromCan() {
    unsigned char len = 0;
    unsigned char buf[8];

    if (CAN_MSGAVAIL == CAN_NISSAN.checkReceive()) {
        CAN_NISSAN.readMsgBuf(&len, buf);

        unsigned long canId = CAN_NISSAN.getCanId();

        // Get the current coolant temperature and engine check light state
        if (canId == 0x551) {
            currentEngineTempCelsius = buf[0] - 40;       // Internet info said -48 from hex byte A but does not line up with 
                                                    // data from NDSIII temperature so -40 it is
        } else if (canId == 0x160) {
            if (buf[6] == 192) {                    // Hex C0, check engine light is off
                if (millis() > (engineCheckTriggeredMillis + (minimumEngineCheckLightDuration * 1000))) {
                    checkEngineLightState = 0;
                }
            }
            if (buf[6] == 224) {                    // Hex E0, check engine light is on
                checkEngineLightState = 2;
                engineCheckTriggeredMillis = millis();
            }
        }
        
        // SERIAL_PORT_MONITOR.print("0x");
        // SERIAL_PORT_MONITOR.print(canId, HEX);
        // SERIAL_PORT_MONITOR.print("\t");

        // for (int i = 0; i < len; i++) { // print the data
        //     SERIAL_PORT_MONITOR.print(buf[i], HEX);
        //     SERIAL_PORT_MONITOR.print("\t");
        // }
        // SERIAL_PORT_MONITOR.println();
    }
}

// Function - Calculate and set radiator fan output
void setRadiatorFanOutput() {
    if (currentEngineTempCelsius >= fanMaximumEngineTemperature) {
        fanPercentageOutput = 100;
    } else if (currentEngineTempCelsius > fanMinimumEngineTemperature) {
        int degreesAboveMinimum = currentEngineTempCelsius - fanMinimumEngineTemperature;
        fanPercentageOutput = (degreesAboveMinimum / (fanMaximumEngineTemperature - fanMinimumEngineTemperature)) * 100;
    } else {
        fanPercentageOutput = 0;
    }

    // Here we will actually set the external PWM control based on calculated percentage output, but only if the engine is running
    if (fanPercentageOutput != 0 && currentRpm > 500) {
        fanPwmPinValue = fanPercentageOutput * 2.55;
    } else {
        fanPwmPinValue = 0;
    }

    // Write out the actual pin value, the pin will maintain this output until the next update
    analogWrite(fanDriverPwmSignalPin, fanPwmPinValue);
}

// Define our pretty tiny scheduler objects
ptScheduler ptCalculateRpm              = ptScheduler(PT_TIME_20MS);
ptScheduler ptCanWriteRpm               = ptScheduler(PT_TIME_10MS);
ptScheduler ptCanWriteTemp              = ptScheduler(PT_TIME_10MS);
ptScheduler ptCanWriteMisc              = ptScheduler(PT_TIME_10MS);
ptScheduler ptSetRadiatorFanOutput      = ptScheduler(PT_TIME_5S);
ptScheduler ptReadEngineElectronicsTemp = ptScheduler(PT_TIME_5S);
ptScheduler ptOutputDebugInfo           = ptScheduler(PT_TIME_1S);

// Our main setup stanza
void setup() {
    SERIAL_PORT_MONITOR.begin(115200);
    while(!Serial){};

    // Configure CAN interfaces
    while (CAN_OK != CAN_BMW.begin(CAN_500KBPS)) {             // init can bus : baudrate = 500k
        SERIAL_PORT_MONITOR.println("BMW CAN init fail, retry...");
        delay(250);
    }
    SERIAL_PORT_MONITOR.println("BMW CAN init ok!");

    while (CAN_OK != CAN_NISSAN.begin(CAN_500KBPS)) {          // init can bus : baudrate = 500k
        SERIAL_PORT_MONITOR.println("Nissan CAN init fail, retry...");
        delay(250);
    }
    SERIAL_PORT_MONITOR.println("Nissan CAN init ok!");

    // Configure interrupt for RPM signal input
    pinMode(rpmSignalPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(rpmSignalPin), updateRpmPulse, RISING);

    // Configure pins for output to fan controller
    pinMode(fanDriverPwmSignalPin, OUTPUT);
    pinMode(fanDriverPwmDirectionPin, OUTPUT);
    digitalWrite(fanDriverPwmDirectionPin, LOW);

    // Configure the temperature sensor
    if (!tempSensorEngineElectronics.begin(0x18)) {
        Serial.println("Couldn't find MCP9808 temp sensor.");
        // while (1);
    } else {
        Serial.println("Found MCP9808 temp sensor.");
        tempSensorEngineElectronics.setResolution(3);
    }

    // Configure masks and filters for Nissan side to reduce noise
    // There are two masks in the mcp2515 which both need to be set
    // Mask 0 has 2 filters and mask 1 has 4 so we set them all as needed
    // 0x551 is where coolant temperature is located
    // 0x160 is **maybe** where the check engine light is
    CAN_NISSAN.init_Mask(0, 0, 0xFFF);
    CAN_NISSAN.init_Filt(0, 0, 0x551);
    CAN_NISSAN.init_Filt(1, 0, 0x160);

    CAN_NISSAN.init_Mask(1, 0, 0xFFF);
    CAN_NISSAN.init_Filt(2, 0, 0x551);
    CAN_NISSAN.init_Filt(3, 0, 0x160);
    CAN_NISSAN.init_Filt(4, 0, 0x551);
    CAN_NISSAN.init_Filt(5, 0, 0x160);
}

// Our main loop
void loop() {
    // Check the pretty tiny scheduler tasks and call as needed
    if (ptCalculateRpm.call()) {
        calculateRpm();
    }

    if (ptCanWriteRpm.call()) {
        canWriteRpm(currentRpm, CAN_BMW);
    }

    if (ptCanWriteTemp.call()) {
        canWriteTemp(currentEngineTempCelsius, CAN_BMW);
    }

    if (ptCanWriteMisc.call()) {
        canWriteMisc();
    }

    if (ptSetRadiatorFanOutput.call()) {
        setRadiatorFanOutput();
    }

    if (ptReadEngineElectronicsTemp.call()) {
        currentEngineElectronicsTemp = readEngineElectronicsTemp(tempSensorEngineElectronics);
    }

    if (ptOutputDebugInfo.call()) {
        outputDebugInfo();
    }

    // Update the values we are looking for from Nissan CAN
    updateNissanDataFromCan();
}
