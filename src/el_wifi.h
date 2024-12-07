#ifndef EL_WIFI_H
#define EL_WIFI_H

#include <WiFiClientSecure.h>

int wifi_init();
int wifi_connect_blocking();
int wifi_disconnect();

int wifi_handle();

bool wifi_isInitialized();
bool wifi_isConnected();
WiFiClientSecure *wifi_getClient();

int wifi_setLoggerOutput(Stream *s);

#endif // EL_WIFI_H