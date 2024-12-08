#ifndef EL_MQTT_H
#define EL_MQTT_H

#include <Arduino.h>
#include <MQTT.h>

int Mqtt_init();
int Mqtt_connect_blocking();
int Mqtt_disconnect();
int Mqtt_handle();
bool Mqtt_isInitialized();
int Mqtt_setCallback(MQTTClientCallbackSimple cb);
int Mqtt_setLoggerOutput(Stream *s);

// Pubublish
int mqtt_publish_light_state(String controller_id, String light_id, String state);

#endif // EL_MQTT_H