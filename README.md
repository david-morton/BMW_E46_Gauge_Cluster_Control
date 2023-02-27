# E46 CAN Bus Cluster + Nissan VQ37 Custom Data Display
This project started as a mechanism to drive my E46 gauge cluster to show correct
data from the Nissan VQ37 I have swapped into the car. The scope has expanded since
I stated out and now does the dash display plus monitoring and display of engine
data which I display on a single high res screen. It's also responsible for 
sending a PWM signal to the radiator fan driver board.

Hardware wise the setup consists of:
- Arduino Mega 2560 R3
- Two Seeed CAN bus shield v2's. One for the BMW car network and one for the Nissan ECM
- A genuine ethernet shield v2 for communicating with the Raspberry Pi
- A Raspberry Pi 4 Model B
- A Waveshare 7.9" capacitive touch display (1280x400 resolution)

High level architecture / detail is:
- Arduino listens for various 'interesting' broadcast messages on the two CAN networks
  like coolant temperature (Nissan ECM) and wheel speeds (BMW ABS).
- Arduino uses an ISR (interrupt service routine) to measure engine RPM directly
  off a wire provided by the ECM
- Arduino directly measures sensor voltages for things that are not tracked by
  the ECM, like oil pressure and crank case vacuum
- Arduino pushes CAN messages onto the BMW network to drive the instrument cluter.
  Primarily these things are RPM, temp, check light and temp warning light
- Arduino pushes MQTT messages via ethernet to the Raspberry Pi which is connected
  with a cross over cable and IP's are statically defined
- Arduino produces a PWM control signal for the radiator fan driver / controller
  which is based on engine temp variance from a target.
- Raspberry Pi runs Grafana server for the display of data and uses the 'grafana live'
  data source which is connected to the Mosquitto MQTT broker which is also running
  locally.
- The Raspberry Pi is configured to run the browser in kiosk mode on boot
  via the grafana-kiosk app
- The Waveshare screen is driven over HDMI from the Raspberry Pi

TODO:
- Vary the screen brightness based on lighting signal voltage