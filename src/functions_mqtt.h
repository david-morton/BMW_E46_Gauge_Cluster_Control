#ifndef FUNCTIONS_MQTT_H
#define FUNCTIONS_MQTT_H

#include <Arduino.h>

/****************************************************
 *
 * Function Prototypes
 *
 ****************************************************/
void connectMqttClientToBroker();
void initialiseEthernetShield();
void publishMqttMetric(String, String, int);
void publishMqttMetric(String, String, String);

#endif
