#ifndef EL_PROTOCOL_H
#define EL_PROTOCOL_H

#include <Arduino.h>

int Protocol_init();
int Protocol_handle();
bool Protocol_isInitialized();
int Protocol_setLoggerOutput(Stream *s);

#endif // EL_PROTOCOL_H