#include <el_wifi.h>
#include <Arduino.h>
#include <WiFiClientSecure.h>

// #======================== Definitions ========================#

#define EASYLIGHT_ENABLE_TLS

// Use private network configuration
#include ".networkConfig.h" // <- Not included in the repository.

#ifndef USE_PRIVATE_NETWORK_CONFIG
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

#ifdef EASYLIGHT_ENABLE_TLS
const char *root_ca = R"literal(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh...
-----END CERTIFICATE-----
)literal";
#endif // EASYLIGHT_ENABLE_TLS

#endif // USE_PRIVATE_NETWORK_CONFIG

// #======================== Global Variables ========================#

WiFiClientSecure wifiClient_s;
Stream *wifiLogger;
bool wifiInitialized = false;

// #======================== Prototypes ========================#

int Wifi_init();
int Wifi_connect_blocking();
int Wifi_disconnect();
int Wifi_handle();
bool Wifi_isInitialized();
bool Wifi_isConnected();
WiFiClientSecure *Wifi_getClient();
int Wifi_setLoggerOutput(Stream *s);

int wifi_log_print(String message);
void cb_wifiConnected();

// #======================== Initialization ========================#

int Wifi_init()
{
  wifi_log_print("[WiFI] Initializing ...\n");
#if defined(EASYLIGHT_ENABLE_TLS)
  wifiClient_s.setCACert(root_ca); // TLS
#endif

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  wifi_log_print("[WiFI] Initialized\n");
  wifiInitialized = true;
  return 0;
}

// #======================== Main ========================#

int Wifi_handle()
{
  if (Wifi_isInitialized() && !Wifi_isConnected())
  {
    wifi_log_print("[WiFI] Reconnecting...\n");
    Wifi_connect_blocking();
  }
  return 0;
}

// #======================== Functions ========================#

int Wifi_connect_blocking()
{
  wifi_log_print("[WiFI] Connecting to SSID: [" + String(WIFI_SSID) + "] ");
  while (WiFi.status() != WL_CONNECTED)
  {
    wifi_log_print(".");
    delay(1000);
  }
  wifi_log_print("\n");
  cb_wifiConnected();
  return 0;
}

int Wifi_disconnect()
{
  WiFi.disconnect();
  return 0;
}

bool Wifi_isInitialized()
{
  return wifiInitialized;
}

bool Wifi_isConnected()
{
  return WiFi.status() == WL_CONNECTED;
}

WiFiClientSecure *Wifi_getClient()
{
  return &wifiClient_s;
}

int Wifi_setLoggerOutput(Stream *s)
{
  wifiLogger = s;
  return 0;
}

int wifi_log_print(String message)
{
  if (wifiLogger)
  {
    wifiLogger->print(message);
  }
  return 0;
}

// #======================== Callbacks ========================#

void cb_wifiConnected()
{
  wifi_log_print("[WiFI] Connected!\n");
  wifi_log_print("[WiFI] IP: " + WiFi.localIP().toString() + "\n");
}

// #======================== Interrupt ========================#

// #======================== End ========================#