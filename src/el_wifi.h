#ifndef EL_WIFI_H
#define EL_WIFI_H

#include <WiFiClientSecure.h>

int wifi_init();
int wifi_connect_blocking();

bool wifi_isInitialized();
WiFiClientSecure *wifi_getClient();

#endif // EL_WIFI_H