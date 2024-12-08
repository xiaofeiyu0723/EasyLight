#ifndef EL_SERVICE_H
#define EL_SERVICE_H

#include <Arduino.h>

int service_init();
int service_handle();
bool service_isInitialized();

int service_setLoggerOutput(Stream *s);

#endif // EL_SERVICE_H