#ifndef EL_SERVICE_H
#define EL_SERVICE_H

#include <Arduino.h>

int Service_init();
int Service_handle();
bool Service_isInitialized();
int Service_setLoggerOutput(Stream *s);

#endif // EL_SERVICE_H