#include <el_wifi.h>
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

WiFiClientSecure Wifi_Client_S;
Stream *Wifi_Logger;
bool wifi_initialized = false;

// #======================== Prototypes ========================#

int wifi_init();
int wifi_connect_blocking();
int wifi_disconnect();

int wifi_handle();

bool wifi_isInitialized();
bool wifi_isConnected();
WiFiClientSecure *wifi_getClient();

int wifi_setLoggerOutput(Stream *s);

// private
int wifi_log_print(String message);
void cb_wifiConnected();

// #======================== Initialization ========================#

int wifi_init()
{
  wifi_log_print("[WiFI] Initializing ...\n");
#if defined(EASYLIGHT_ENABLE_TLS)
  Wifi_Client_S.setCACert(root_ca); // TLS
#endif

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  wifi_log_print("[WiFI] Initialized\n");
  wifi_initialized = true;
  return 0;
}

// #======================== Main ========================#

int wifi_handle()
{
  if (wifi_isInitialized() && !wifi_isConnected())
  {
    wifi_log_print("[WiFI] Reconnecting...\n");
    wifi_connect_blocking();
  }
  return 0;
}

// #======================== Functions ========================#

int wifi_connect_blocking()
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

int wifi_disconnect()
{
  WiFi.disconnect();
  return 0;
}

bool wifi_isInitialized()
{
  return wifi_initialized;
}

bool wifi_isConnected()
{
  return WiFi.status() == WL_CONNECTED;
}

WiFiClientSecure *wifi_getClient()
{
  return &Wifi_Client_S;
}

int wifi_setLoggerOutput(Stream *s)
{
  Wifi_Logger = s;
  return 0;
}

// private
int wifi_log_print(String message)
{
  if (Wifi_Logger)
  {
    Wifi_Logger->print(message);
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