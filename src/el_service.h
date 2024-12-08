#ifndef EL_SERVICE_H
#define EL_SERVICE_H

#include <Arduino.h>

int Service_init();
int Service_handle();
bool Service_isInitialized();
int Service_setLoggerOutput(Stream *s);

int Service_updateLightState(String controller_id, String light_id, String state);

#endif // EL_SERVICE_H