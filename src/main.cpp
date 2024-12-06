#include <WiFiClientSecure.h>
#include <MQTT.h>

// #======================== Definitions ========================#

#define SERIAL_BAUD 115200
#define EASYLIGHT_ENABLE_TLS // Enable TLS

// Use private network configuration
#include ".networkConfig/networkConfig.h" // <- Not included in the repository.

#ifndef USE_PRIVATE_NETWORK_CONFIG // -----
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

#define MQTT_HOST "YOUR_MQTT_HOST"
#define MQTT_PORT 8883
#define MQTT_USERNAME "YOUR_MQTT_USERNAME"
#define MQTT_PASSWORD "YOUR_MQTT_PASSWORD"

#ifdef EASYLIGHT_ENABLE_TLS
const char *root_ca = R"literal(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh...
-----END CERTIFICATE-----
)literal";
#endif // EASYLIGHT_ENABLE_TLS
#endif // USE_PRIVATE_NETWORK_CONFIG ----- [copy above to .networkConfig/networkConfig.h and fill in your data]

// #======================== Global Variables ========================#

WiFiClientSecure Wifi_Client_S;
MQTTClient MQTT_Client;

unsigned long lastMillis = 0;
String Client_ID = "el_" + String(ESP.getEfuseMac(), HEX); // Unique ID 'el_xx9fxxefxxc0'

// #======================== Prototypes ========================#

void startConnect();

void wifi_init();
void mqtt_init();

void wifi_connect_blocking();
void mqtt_connect_blocking();

void cb_wifiConnected();
void cb_mqttConnected();
void cb_mqttMessageReceived(String &topic, String &payload);

// #======================== Initialization ========================#

void setup()
{
  Serial.begin(SERIAL_BAUD);

  wifi_init();
  mqtt_init();

  startConnect();
}

// #======================== Main Loop ========================#

void loop()
{
  MQTT_Client.loop();
  delay(10); // <- fixes some issues with WiFi stability

  if (!MQTT_Client.connected())
  {
    startConnect();
  }

  // publish a message roughly every second.
  if (millis() - lastMillis > 3000)
  {
    lastMillis = millis();
    MQTT_Client.publish("hello", "world");
  }
}

// #======================== Functions ========================#

void startConnect()
{
  wifi_connect_blocking();

#if defined(EASYLIGHT_ENABLE_TLS)
  Wifi_Client_S.setCACert(root_ca); // TLS
#endif

  mqtt_connect_blocking();
}

void wifi_init()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void mqtt_init()
{
  MQTT_Client.begin(MQTT_HOST, MQTT_PORT, Wifi_Client_S);
  MQTT_Client.onMessage(cb_mqttMessageReceived); // cb
}

void wifi_connect_blocking()
{
  Serial.print("[WiFI] Connecting to SSID: [" + String(WIFI_SSID) + "] ");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
  }
  cb_wifiConnected();
}

void mqtt_connect_blocking()
{
  Serial.println("[MQTT] Client ID: " + Client_ID);
  // Serial.print("[MQTT] Connecting to Host: [" + String(MQTT_HOST) + "] ");
  Serial.print("[MQTT] Connecting ");

  MQTT_Client.setWill(("easylight/" + Client_ID + "/$state").c_str(), "lost", true, 2); // LWT set brfore connect
  while (!MQTT_Client.connect(Client_ID.c_str(), MQTT_USERNAME, MQTT_PASSWORD))
  {
    Serial.print(".");
    delay(1000);
  }
  cb_mqttConnected();
}

// #======================== Callbacks ========================#

void cb_wifiConnected()
{
  Serial.println("\n[WiFI] Connected!");
  Serial.println("[WiFI] IP: " + WiFi.localIP().toString());
}

void cb_mqttConnected()
{
  Serial.println("\n[MQTT] Connected!");
  MQTT_Client.subscribe("hello");
}

void cb_mqttMessageReceived(String &topic, String &payload)
{
  Serial.println("[MQTT]incoming: " + topic + " - " + payload);

  // Note: Do not use the MQTT_Client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `MQTT_Client.loop()`.
}

// #======================== Interrupt ========================#

// #======================== End ========================#