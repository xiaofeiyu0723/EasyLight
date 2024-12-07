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
bool wifi_initialized = false;

// #======================== Prototypes ========================#

int wifi_init();
int wifi_connect_blocking();

bool wifi_isInitialized();
WiFiClientSecure *wifi_getClient();

// #======================== Initialization ========================#

int wifi_init()
{
#if defined(EASYLIGHT_ENABLE_TLS)
  Wifi_Client_S.setCACert(root_ca); // TLS
#endif

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  wifi_initialized = true;
  return 0;
}

// #======================== Main ========================#

int wifi_connect_blocking()
{
  Serial.print("[WiFI] Connecting to SSID: [" + String(WIFI_SSID) + "] ");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
  }
  //   cb_wifiConnected();
  return 0;
}

// #======================== Functions ========================#

bool wifi_isInitialized()
{
  return wifi_initialized;
}

WiFiClientSecure *wifi_getClient()
{
  return &Wifi_Client_S;
}

// #======================== Callbacks ========================#

void cb_wifiConnected()
{
  Serial.println("\n[WiFI] Connected!");
  Serial.println("[WiFI] IP: " + WiFi.localIP().toString());
}

// #======================== Interrupt ========================#

// #======================== End ========================#