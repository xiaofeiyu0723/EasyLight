#ifndef EL_PROTOCOL_H
#define EL_PROTOCOL_H

#include <Arduino.h>

int protocol_init();
int protocol_handle();
bool protocol_isInitialized();

int protocol_setLoggerOutput(Stream *s);

#endif // EL_PROTOCOL_H