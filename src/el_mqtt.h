#ifndef EL_MQTT_H
#define EL_MQTT_H

#include <MQTT.h>

int mqtt_init();
int mqtt_connect_blocking();
int mqtt_disconnect();

int mqtt_handle();
int mqtt_setCallback(MQTTClientCallbackSimple cb);

#endif // EL_MQTT_H