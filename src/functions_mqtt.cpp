#include "functions_mqtt.h"

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>   // MQTT Client library

// Assign the slave select pin. This is pin 10 on the shield which is bent and jumpered
const int ETH_SS_PIN = 31;

// Define toggle for connection state
bool mqttBrokerConnected = false;

// Configure ethernet and MQTT pieces
byte eth_mac[] = {  0xA8, 0x61, 0x0A, 0xAE, 0xAB, 0x8D }; // Define the ethernet shielf MAC
byte eth_ip[] = { 192, 168, 11, 3 };                      // Define the ethernet shield IP
IPAddress mqtt_server(192, 168, 11, 2);                   // Grafana server address on Raspberry Pi
const int mqtt_port = 1883;                               // Grafana server port on Raspberry Pi
EthernetClient eth_client;                                // Create ethernet client
PubSubClient mqttClient(eth_client);                      // Create MQTT client on ethernet

// Function for setting up the ethernet shield
void initialiseEthernetShield() {
  SERIAL_PORT_MONITOR.println("INFO - Initialising ethernet shield");
  Ethernet.init(ETH_SS_PIN);
  Ethernet.begin(eth_mac, eth_ip);

  char eth_status = Ethernet.hardwareStatus();

  if (eth_status == EthernetW5500) {
    SERIAL_PORT_MONITOR.println("\tOK - W5500 Ethernet controller detected");
  } else if (eth_status != EthernetW5500) {
    SERIAL_PORT_MONITOR.print("\tFATAL - Ethernet status is ");
    SERIAL_PORT_MONITOR.println(eth_status);
  }
}

// Function for creating the MQTT client and connecting to server
void connectMqttClientToBroker() {
    if (!mqttClient.connected()) {
        SERIAL_PORT_MONITOR.println("INFO - Connecting to MQTT broker");
        mqttClient.setServer(mqtt_server, mqtt_port);
        mqttClient.setKeepAlive(5);
        if (mqttClient.connect("arduino-client")) {
            SERIAL_PORT_MONITOR.println("\tOK - MQTT Client connected");
            mqttBrokerConnected = true;
        } else {
            SERIAL_PORT_MONITOR.println("\tFATAL - MQTT Client not connected");
            mqttBrokerConnected = false;
        }
    }
}

// Publish metric via MQTT
void publishMqttMetric(String topic, String metricName, int metricValue) {
  if (mqttBrokerConnected) {
    String payload = "{\"" + metricName + "\":" + String(metricValue) + "}";
    mqttClient.publish(topic.c_str(), payload.c_str());
  }
}

void publishMqttMetric(String topic, String metricName, String metricValue) {
  if (mqttBrokerConnected) {
    String payload = "{\"" + metricName + "\":" + String(metricValue) + "}";
    mqttClient.publish(topic.c_str(), payload.c_str());
  }
}