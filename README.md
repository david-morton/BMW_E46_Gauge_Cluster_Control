# Project Purpose & Description
This code is used for various aspects of integrating a Nissan VQ37 engine (using
the OEM engine computer) with a 2000 BMW E46 as part of an engine swap project.

The ECU used is from a USDM 2011 manual 370Z and the car is a right hand drive.

The aim is to create an OEM-like experience where the engine is integrated as naturally
as possible with the car. On top of this basic requirement, we also introduce a number
of custom functions. Features in each category are described below.

### OEM Like functionality:
- RPM gauge (custom calibration for extended RPM gauge faces)
- Temperature gauge
- Temperature warning light
- Check engine and EML light control
- Fuel consumption meter

### Additional functionality:
- Control of radiator fan via PWM signal
- Interface with various automotive sensors
- Custom LCD data display (using Grafana Live)
- Display of various performance metrics (best 0-100 / 80-120 etc)
- Sounding of alarm buzzer (loss of oil pressure etc)
- Polling of real time ECU parameters

# Hardware Used
The hardware used for my particular appliation is:
- Arduino Mega 2560 R3
  - Where this code is run
- 2x Seeed CAN-Bus shield v2's
  - One for the BMW / car network
  - One for the Nissan / engine network
- A genuine ethernet shield v2
  - For MQTT communication between Arduino and Orange Pi
- Orange Pi 5
  - For running the Waveshare display over HDMI
- A Waveshare 7.9" capacitive touch display (1280x400 resolution)
- Various ProSport automotive sensors which interface with the Arduino
  - Fuel pressure
  - Oil pressure
  - Oil temperature
  - Coolant temperature
  - Crank case vacuum
- Cytron MD30C PWM motor controller
  - Used to drive the radiator fan via Arduino PWM signal
  - Capable of driving a brushed DC motor at constant 30A

# Implementation Detail & Architecture
- Arduino listens for various 'interesting' broadcast messages on the two CAN networks
  like coolant temperature (Nissan ECU) and wheel speeds (BMW ABS). It then re-broadcasts
  messages to the appropriate location. 
  - Example 1: Engine temp is read from the Nissan CAN network and written to the BMW CAN network
  for the gauge cluster to display
  - Example 2: Vehicle wheel speeds are read from the BMW CAN network and written to the Nissan
  CAN network for the ECU to know the vehicle speed
- Arduino uses an ISR (interrupt service routine) to measure engine RPM directly
  off a signal wire provided by the ECU
- Arduino directly measures sensor voltages for things that are not tracked by
  the ECU, like oil pressure and crank case vacuum. These are done via a basic
  voltage divider and using a dedicated 5V supply
- Arduino pushes MQTT messages via ethernet to the Orange Pi which is connected
  via a cross over cable. IP's are statically defined on both interfaces
- Orange Pi runs Grafana server for the display of data and uses the 'grafana live'
  data source which is connected to the Mosquitto MQTT broker which is also running
  locally on the Pi. A custom dash is created in Grafana to display various streamed data
- The Orange Pi is configured to run the browser in kiosk mode on boot via the grafana-kiosk app

# Technical Notes
The fuel economy gauge only works when there is a speed input.

Inputting a square wave of 410Hz and 50% duty cycle gives 60km/h
on the test gauge cluster. This signal is put into pin 19 on cluster
connector X11175 for testing purposes on the bench. This signal is
usually provided by the ABS computer, which reads the 4 wheel speed
sensors.

The lowest speed pulse generation that seems to allow activation of the fuel
economy gauge is 82Hz when increasing and it will stop function at 68Hz when
decreasing. 82Hz seems to be about 5kph.

The VQ37 ECU pin 110 is 'Engine speed output signal' and outputs a square
wave at 3 pulses per revolution. We use this signal to calculate the engine
RPM and send the value to the cluster.

# Todo
- Vary the Waveshare screen brightness based on ambient light sensor
- Implement the screens touch capability to cycle through dashboards
- Implement front / rear cameras to help with parking and interface
  with the screen