#ifndef EL_RADIO_H
#define EL_RADIO_H

#include <Arduino.h>

int Radio_init();
int Radio_handle();
bool Radio_isInitialized();
int Radio_setReceiveCallback(void (*cb)(byte *packet, size_t len));
int Radio_setLoggerOutput(Stream *s);

#endif // EL_RADIO_H