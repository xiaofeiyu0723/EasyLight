#ifndef EL_MQTT_H
#define EL_MQTT_H

#include <Arduino.h>
#include <MQTT.h>

int Mqtt_init();
int Mqtt_connect_blocking();
int Mqtt_disconnect();
int Mqtt_handle();
int Mqtt_publish(String topic, String payload, bool retained = false, int qos = 0);
bool Mqtt_isInitialized();
bool Mqtt_isConnected();
MQTTClient *Mqtt_getClient();
int Mqtt_setClientID(String id);
int Mqtt_setRootTopic(String topic);
int Mqtt_setCallback(MQTTClientCallbackSimple cb);
int Mqtt_setLoggerOutput(Stream *s);

#endif // EL_MQTT_H