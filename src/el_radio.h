#ifndef EL_RADIO_H
#define EL_RADIO_H

#include <Arduino.h>

int radio_init();
int radio_handle();

bool radio_isInitialized();

int radio_setReceiveCallback(void (*cb)(byte *packet, size_t len));
int radio_setLoggerOutput(Stream *s);

#endif // EL_RADIO_H