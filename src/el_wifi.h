#ifndef EL_WIFI_H
#define EL_WIFI_H

#include <Arduino.h>
#include <WiFiClientSecure.h>

int Wifi_init();
int Wifi_connect_blocking();
int Wifi_disconnect();
int Wifi_handle();
bool Wifi_isInitialized();
bool Wifi_isConnected();
WiFiClientSecure *Wifi_getClient();
int Wifi_setLoggerOutput(Stream *s);

#endif // EL_WIFI_H