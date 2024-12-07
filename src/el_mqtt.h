#ifndef EL_MQTT_H
#define EL_MQTT_H

#include <MQTT.h>

int mqtt_init();
int mqtt_connect_blocking();
int mqtt_disconnect();

int mqtt_handle();

bool mqtt_isInitialized();

int mqtt_setCallback(MQTTClientCallbackSimple cb);
int mqtt_setLoggerOutput(Stream *s);

// Pubublish
int mqtt_publish_light_state(String controller_id, String light_id, String state);

#endif // EL_MQTT_H