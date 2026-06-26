/*
  EasyLight Bemfa Gateway for ESP8266 D1 mini

  D1 mini + CC1101 bridge:
    Bemfa/Mijia/Xiaoai switch command -> EasyLight 433 MHz controller packet

  Features:
    - Syncs Bemfa switch topics from the cloud API
    - Serves a small pairing web portal over a setup hotspot
    - Captures controller IDs from RF responses and saves topic mappings to EEPROM

  Required libraries:
    - PubSubClient
    - RadioLib
    - AceCRC
    - ArduinoJson
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <RadioLib.h>
#include <AceCRC.h>
#include <ArduinoJson.h>

#include "bemfa_gateway_esp8266_config.h"

using namespace ace_crc::crc16ccitt_byte;

// Bemfa MQTT
#define BEMFA_MQTT_HOST "bemfa.com"
#define BEMFA_MQTT_PORT 9501

// D1 mini / ESP8266 + CC1101 pinout
#define PIN_CS 15    // D8
#define PIN_GDO0 5   // D1
#define PIN_RST RADIOLIB_NC
#define PIN_GDO2 4   // D2

// RF configuration
#define RADIO_CARRIER_FREQUENCY 433.3
#define RADIO_BIT_RATE 250.0
#define RADIO_FREQUENCY_DEVIATION 125.0
#define RADIO_RX_BANDWIDTH 270.0
#define RADIO_OUTPUT_POWER 10
#define RADIO_PREAMBLE_LENGTH 32
#define PACKET_LENGTH 24
#define PACKET_THIRD_SYNC_WORD 0x23

// Web pairing portal
#define DNS_PORT 53
#define HTTP_PORT 80
#define EEPROM_SIZE 4096
#define PAIR_WINDOW_MS 60000UL
#define WIFI_RECONNECT_MS 15000UL
#define WIFI_CONNECT_TIMEOUT_MS 30000UL
#define POWER_ACK_TIMEOUT_MS 2000UL
#define POWER_ACK_RETRY_DELAY_MS 2000UL
#define POWER_ACK_MAX_RETRIES 1
#define RF_TX_GAP_MS 750UL
#define RF_POWER_QUEUE_SIZE 12
#define POWER_ACK_TRACKER_SIZE 8
#define MAX_LIGHT_BINDINGS 24
#ifndef SETUP_AP_PASSWORD
#define SETUP_AP_PASSWORD "easylight"
#endif

#ifndef STATIC_LIGHT_BINDINGS
#define STATIC_LIGHT_BINDINGS {"", "", ""}
#endif

struct StaticLightBinding
{
  const char *topic;
  const char *controllerId;
  const char *label;
};

struct LightBinding
{
  String topic;
  String label;
};

struct QueuedPowerCommand
{
  bool active;
  String topic;
  byte controllerId[3];
  bool powerOn;
  byte attempt;
  unsigned long dueAt;
};

struct PendingPowerAck
{
  bool active;
  String topic;
  byte controllerId[3];
  bool powerOn;
  byte attempt;
  unsigned long sentAt;
};

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
CC1101 radio = new Module(PIN_CS, PIN_GDO0, PIN_RST, PIN_GDO2);
ESP8266WebServer webServer(HTTP_PORT);
DNSServer dnsServer;

StaticLightBinding staticLightBindings[] = {STATIC_LIGHT_BINDINGS};
const size_t staticLightBindingCount = sizeof(staticLightBindings) / sizeof(staticLightBindings[0]);
LightBinding lightBindings[MAX_LIGHT_BINDINGS];
String runtimeControllerIds[MAX_LIGHT_BINDINGS];
size_t lightBindingCount = 0;
QueuedPowerCommand powerQueue[RF_POWER_QUEUE_SIZE];
PendingPowerAck pendingPowerAcks[POWER_ACK_TRACKER_SIZE];

uint8_t syncWord[] = {0x21, 0xA4};
unsigned long lastMqttReconnectAttempt = 0;
unsigned long lastWifiReconnectAttempt = 0;
unsigned long wifiConnectStartedAt = 0;
unsigned long lastStatusPublishAt = 0;
unsigned long pairingStartedAt = 0;
unsigned long nextRfTxAt = 0;
int pairingTopicIndex = -1;
int lastWifiStatus = WL_IDLE_STATUS;
bool radioReady = false;
bool pairingActive = false;
bool wifiWasConnected = false;
bool mqttWasConnected = false;
bool bemfaAutoSyncDone = false;
String setupApName;
String deviceHostName;
String radioStatusText = "not initialized";
String cloudSyncStatus = "not synced";
String configuredWifiSsid;
String configuredWifiPassword;

volatile bool receivedFlag = false;

#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void setReceivedFlag()
{
  receivedFlag = true;
}

void startSetupPortal();
void handleRoot();
void handleWifiSave();
void handleSyncDevices();
void handlePair();
void handleUnpair();
void handleClear();
void handleNotFound();
String renderPage();
String htmlEscape(const String &value);
void startPairing(int index);
void stopPairing(const char *reason);
void handleRadio();
bool extractControllerId(byte *packet, size_t len, byte controllerId[3]);
bool validateControllerResponse(byte *packet, size_t len);
void logGatewayResponse(byte *packet, byte controllerId[3]);
const char *gatewayCommandName(byte command);
void saveCapturedController(byte controllerId[3]);
void startPowerCommand(const String &topic, byte controllerId[3], bool powerOn);
bool enqueuePowerCommand(const String &topic, byte controllerId[3], bool powerOn, byte attempt, unsigned long dueAt);
void cancelPendingPowerAck(byte controllerId[3]);
void trackPowerAck(const String &topic, byte controllerId[3], bool powerOn, byte attempt);
void maintainPowerQueue();
void maintainPowerAcks();
void maintainPowerCommand();
bool handlePowerAck(byte *packet, byte controllerId[3]);

void beginWifi();
void maintainWifi();
void startWifiConnection(const char *reason);
const char *wifiStatusName(int status);
bool connectMqtt();
void subscribeTopics();
void maintainMqtt();
void handleMqtt(char *topic, byte *payload, unsigned int length);

void initBindingsFromConfig();
bool syncBemfaDevices();
void addOrUpdateBinding(const String &topic, const String &label);
bool shouldUseBemfaDevice(const String &topic, const String &deviceType);
void loadMappings();
void saveMappings();
void initWifiConfig();
String hexEncode(const String &value);
String hexDecode(const String &hex);
int hexNibble(char c);
int findBindingByTopic(const String &topic);
bool parseControllerId(const String &hex, byte controllerId[3]);
String controllerIdToHex(byte controllerId[3]);

bool configureReceiveMode(bool updateStatus = true);
bool sendPowerPacket(byte controllerId[3], bool powerOn);
bool sendAddGatewayPacket(byte controllerId[3]);
bool sendDeleteGatewayPacket(byte controllerId[3]);
bool sendGatewayBindingPacket(byte controllerId[3], byte command, const char *label);
bool transmitPacket(byte *packet, size_t len);
bool controllerIdsMatch(byte left[3], byte right[3]);
void placeCrc(byte *packet, const size_t *payloadIndexes, size_t payloadLen, size_t crcHighIndex, size_t crcLowIndex);
void publishExpectedState(const String &topic, bool powerOn);
void publishGatewayStatus(bool force = false);
String statusTopic();
void printTopicMappings();

void setup()
{
  Serial.begin(115200);
  delay(100);

  Serial.println();
  Serial.println("[EasyLight] Bemfa ESP8266 gateway starting");

  EEPROM.begin(EEPROM_SIZE);
  initWifiConfig();
  initBindingsFromConfig();
  loadMappings();
  printTopicMappings();

  startSetupPortal();
  beginWifi();

  Serial.print("[CC1101] Initializing ... ");
  int state = radio.begin(
      RADIO_CARRIER_FREQUENCY,
      RADIO_BIT_RATE,
      RADIO_FREQUENCY_DEVIATION,
      RADIO_RX_BANDWIDTH,
      RADIO_OUTPUT_POWER,
      RADIO_PREAMBLE_LENGTH);

  radio.setCrcFiltering(false);
  radio.setPacketReceivedAction(setReceivedFlag);

  if (state == RADIOLIB_ERR_NONE)
  {
    radioReady = true;
    Serial.println("success");
    configureReceiveMode();
  }
  else
  {
    radioStatusText = "error:" + String(state);
    Serial.print("failed, code ");
    Serial.println(state);
    Serial.println("[CC1101] Web/MQTT will still run for cloud testing.");
  }

  mqttClient.setServer(BEMFA_MQTT_HOST, BEMFA_MQTT_PORT);
  mqttClient.setBufferSize(512);
  mqttClient.setKeepAlive(60);
  mqttClient.setSocketTimeout(15);
  mqttClient.setCallback(handleMqtt);
}

void loop()
{
  dnsServer.processNextRequest();
  webServer.handleClient();

  maintainWifi();
  maintainMqtt();
  publishGatewayStatus(false);
  handleRadio();
  maintainPowerCommand();

  if (pairingActive && millis() - pairingStartedAt > PAIR_WINDOW_MS)
  {
    stopPairing("timeout");
  }

  yield();
}

void startSetupPortal()
{
  setupApName = "EasyLight-";
  setupApName += String(ESP.getChipId(), HEX);
  setupApName.toUpperCase();
  deviceHostName = "easylight-";
  deviceHostName += String(ESP.getChipId(), HEX);
  deviceHostName.toLowerCase();

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(setupApName.c_str(), SETUP_AP_PASSWORD);

  IPAddress apIp = WiFi.softAPIP();
  dnsServer.start(DNS_PORT, "*", apIp);

  webServer.on("/", handleRoot);
  webServer.on("/wifi", HTTP_POST, handleWifiSave);
  webServer.on("/sync", handleSyncDevices);
  webServer.on("/pair", handlePair);
  webServer.on("/unpair", handleUnpair);
  webServer.on("/clear", handleClear);
  webServer.onNotFound(handleNotFound);
  webServer.begin();

  Serial.print("[Portal] AP: ");
  Serial.print(setupApName);
  Serial.print(" password: ");
  Serial.println(SETUP_AP_PASSWORD);
  Serial.print("[Portal] URL: http://");
  Serial.println(apIp);
}

void handleRoot()
{
  webServer.send(200, "text/html; charset=utf-8", renderPage());
}

void handleWifiSave()
{
  String ssid = webServer.arg("ssid");
  String password = webServer.arg("password");
  ssid.trim();

  if (ssid.length() == 0)
  {
    webServer.send(400, "text/plain", "SSID is required");
    return;
  }

  configuredWifiSsid = ssid;
  configuredWifiPassword = password;
  saveMappings();

  Serial.print("[WiFi] Saved SSID: ");
  Serial.println(configuredWifiSsid);
  bemfaAutoSyncDone = false;
  WiFi.disconnect(false);
  startWifiConnection("settings saved");

  webServer.sendHeader("Location", "/", true);
  webServer.send(302, "text/plain", "");
}

void handleSyncDevices()
{
  if (syncBemfaDevices())
  {
    subscribeTopics();
  }

  webServer.sendHeader("Location", "/", true);
  webServer.send(302, "text/plain", "");
}

void handlePair()
{
  int index = webServer.arg("i").toInt();
  if (index < 0 || (size_t)index >= lightBindingCount)
  {
    webServer.send(400, "text/plain", "Invalid topic index");
    return;
  }

  startPairing(index);
  webServer.sendHeader("Location", "/", true);
  webServer.send(302, "text/plain", "");
}

void handleClear()
{
  handleUnpair();
}

void handleUnpair()
{
  int index = webServer.arg("i").toInt();
  if (index < 0 || (size_t)index >= lightBindingCount)
  {
    webServer.send(400, "text/plain", "Invalid topic index");
    return;
  }

  byte controllerId[3];
  bool hadControllerId = parseControllerId(runtimeControllerIds[index], controllerId);
  if (hadControllerId)
  {
    if (!sendDeleteGatewayPacket(controllerId))
    {
      Serial.print("[Portal] Unpair RF DELETE failed for ");
      Serial.println(lightBindings[index].topic);
      webServer.sendHeader("Location", "/", true);
      webServer.send(302, "text/plain", "");
      return;
    }
  }

  runtimeControllerIds[index] = "";
  saveMappings();
  if (pairingActive && pairingTopicIndex == index)
  {
    stopPairing("unpaired");
  }

  Serial.print("[Portal] Unpaired ");
  Serial.println(lightBindings[index].topic);
  webServer.sendHeader("Location", "/", true);
  webServer.send(302, "text/plain", "");
}

void handleNotFound()
{
  webServer.sendHeader("Location", "/", true);
  webServer.send(302, "text/plain", "");
}

String renderPage()
{
  String html;
  html.reserve(9000);
  html += F("<!doctype html><html><head><meta charset='utf-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  if (pairingActive)
  {
    html += F("<meta http-equiv='refresh' content='3'>");
  }
  html += F("<title>EasyLight Pairing</title>");
  html += F("<style>");
  html += F("body{font-family:Arial,sans-serif;margin:20px;background:#f5f7fb;color:#162033}");
  html += F("h1{font-size:24px;margin:0 0 12px}.status{padding:12px;background:white;border-radius:8px;margin:12px 0}");
  html += F(".card{background:white;border-radius:8px;padding:14px;margin:12px 0;box-shadow:0 1px 4px #d7dce8}");
  html += F(".topic{font-weight:700;font-size:18px}.id{color:#667085;margin:8px 0}");
  html += F("a.btn{display:inline-block;padding:10px 14px;border-radius:6px;background:#2764d8;color:white;text-decoration:none;margin-right:8px}");
  html += F("button.btn{border:0;padding:10px 14px;border-radius:6px;background:#2764d8;color:white;font-size:14px}");
  html += F("input{box-sizing:border-box;width:100%;padding:10px;border:1px solid #ccd3df;border-radius:6px;margin:6px 0 10px;font-size:15px}");
  html += F("label{display:block;font-size:13px;color:#667085;margin-top:8px}");
  html += F("a.clear{background:#8b98aa}.danger{color:#bd2b2b}.small{font-size:13px;color:#667085}");
  html += F("</style></head><body>");
  html += F("<h1>EasyLight Pairing</h1>");

  html += F("<div class='status'>");
  html += F("<div>AP: <b>");
  html += htmlEscape(setupApName);
  html += F("</b></div>");
  if (WiFi.status() == WL_CONNECTED)
  {
    html += F("<div>URL: <b>http://");
    html += WiFi.localIP().toString();
    html += F("</b></div>");
  }
  html += F("<div>AP URL: <b>http://192.168.4.1</b></div>");
  html += F("<div>WiFi: <b>");
  int wifiStatus = WiFi.status();
  if (wifiStatus == WL_CONNECTED)
  {
    html += F("connected");
  }
  else
  {
    html += htmlEscape(wifiStatusName(wifiStatus));
  }
  html += F("</b></div>");
  html += F("<div>SSID: <b>");
  html += htmlEscape(configuredWifiSsid);
  html += F("</b></div>");
  if (WiFi.status() == WL_CONNECTED)
  {
    html += F("<div>IP: <b>");
    html += WiFi.localIP().toString();
    html += F("</b></div>");
    html += F("<div>RSSI: <b>");
    html += String(WiFi.RSSI());
    html += F(" dBm</b></div>");
  }
  html += F("<div>MQTT: <b>");
  html += mqttClient.connected() ? "connected" : "not connected";
  html += F("</b></div>");
  html += F("<div>Bemfa sync: <b>");
  html += htmlEscape(cloudSyncStatus);
  html += F("</b></div>");
  html += F("<div>Radio: <b>");
  html += htmlEscape(radioStatusText);
  html += F("</b></div>");
  html += F("<div>Status topic: <b>");
  html += htmlEscape(statusTopic());
  html += F("</b></div>");
  if (pairingActive && pairingTopicIndex >= 0)
  {
    unsigned long remaining = (PAIR_WINDOW_MS - (millis() - pairingStartedAt)) / 1000;
    html += F("<div class='danger'>Pairing: ");
    html += htmlEscape(lightBindings[pairingTopicIndex].topic);
    html += F(" (");
    html += String(remaining);
    html += F("s left). Trigger the target controller now.</div>");
  }
  html += F("</div>");

  html += F("<div class='card'>");
  html += F("<div class='topic'>WiFi</div>");
  html += F("<form method='post' action='/wifi'>");
  html += F("<label>SSID</label>");
  html += F("<input name='ssid' value='");
  html += htmlEscape(configuredWifiSsid);
  html += F("' maxlength='32'>");
  html += F("<label>Password</label>");
  html += F("<input name='password' type='password' maxlength='64'>");
  html += F("<button class='btn' type='submit'>Save WiFi</button>");
  html += F("</form></div>");

  html += F("<div class='card'>");
  html += F("<div class='topic'>Bemfa Devices</div>");
  html += F("<div class='id'>");
  html += htmlEscape(cloudSyncStatus);
  html += F("</div>");
  html += F("<a class='btn' href='/sync'>Sync Bemfa</a>");
  html += F("</div>");

  for (size_t i = 0; i < lightBindingCount; i++)
  {
    String topic = lightBindings[i].topic;
    topic.trim();
    if (topic.length() == 0)
    {
      continue;
    }

    html += F("<div class='card'>");
    html += F("<div class='topic'>");
    String label = lightBindings[i].label;
    label.trim();
    html += htmlEscape(label.length() ? label : topic);
    html += F("</div>");
    html += F("<div style='margin:10px 0'>");
    html += F("<a class='btn' href='/pair?i=");
    html += String(i);
    html += F("'>Pair</a>");
    html += F("<a class='btn clear' href='/unpair?i=");
    html += String(i);
    html += F("'>Unpair</a>");
    html += F("</div>");
    html += F("<div class='id'>Controller ID: ");
    html += runtimeControllerIds[i].length() ? htmlEscape(runtimeControllerIds[i]) : "(not paired)";
    html += F("</div><div class='small'>Topic: ");
    html += htmlEscape(topic);
    html += F("</div>");
    html += F("</div>");
  }

  html += F("<p class='small'>Pairing flow: tap Pair, then trigger the light/controller within 60 seconds. Unpair sends the RF DELETE packet and clears the saved mapping.</p>");
  html += F("</body></html>");
  return html;
}

String htmlEscape(const String &value)
{
  String escaped = value;
  escaped.replace("&", "&amp;");
  escaped.replace("<", "&lt;");
  escaped.replace(">", "&gt;");
  escaped.replace("\"", "&quot;");
  escaped.replace("'", "&#39;");
  return escaped;
}

void startPairing(int index)
{
  pairingTopicIndex = index;
  pairingStartedAt = millis();
  pairingActive = true;

  Serial.print("[Pairing] Started for ");
  Serial.println(lightBindings[index].topic);
  if (radioReady)
  {
    configureReceiveMode();
  }
}

void stopPairing(const char *reason)
{
  if (!pairingActive)
  {
    return;
  }

  Serial.print("[Pairing] Stopped: ");
  Serial.println(reason);
  pairingActive = false;
  pairingTopicIndex = -1;
}

void handleRadio()
{
  if (!radioReady || !receivedFlag)
  {
    return;
  }

  receivedFlag = false;
  byte packet[PACKET_LENGTH];
  int state = radio.readData(packet, PACKET_LENGTH);

  if (state != RADIOLIB_ERR_NONE)
  {
    radioStatusText = "receive error:" + String(state);
    Serial.print("[RF] Receive failed, code ");
    Serial.println(state);
    configureReceiveMode();
    return;
  }

  byte controllerId[3];
  if (!extractControllerId(packet, PACKET_LENGTH, controllerId))
  {
    configureReceiveMode();
    return;
  }

  Serial.print("[RF] Controller response from ");
  Serial.println(controllerIdToHex(controllerId));
  logGatewayResponse(packet, controllerId);
  handlePowerAck(packet, controllerId);

  if (pairingActive && pairingTopicIndex >= 0)
  {
    saveCapturedController(controllerId);
  }

  configureReceiveMode(false);
}

bool extractControllerId(byte *packet, size_t len, byte controllerId[3])
{
  if (len < 13 || packet[0] != PACKET_THIRD_SYNC_WORD)
  {
    return false;
  }

  byte msgType = packet[1];
  if (msgType != 0x01 && msgType != 0x04)
  {
    return false;
  }

  byte effectiveLen = packet[2];
  if (effectiveLen < 0x0B || effectiveLen + 1 > len)
  {
    return false;
  }

  if (!validateControllerResponse(packet, len))
  {
    Serial.println("[RF] Controller response CRC invalid");
    return false;
  }

  controllerId[0] = packet[4];
  controllerId[1] = packet[5];
  controllerId[2] = packet[9];
  return true;
}

bool validateControllerResponse(byte *packet, size_t len)
{
  if (len < 13)
  {
    return false;
  }

  byte effectiveLen = packet[2];
  if (effectiveLen + 1 > len || effectiveLen < 0x0B)
  {
    return false;
  }

  byte payload[PACKET_LENGTH];
  size_t payloadLen = 0;

  for (int i = 1; i <= 5; i++)
  {
    payload[payloadLen++] = packet[i];
  }

  for (int i = 9; i <= effectiveLen; i++)
  {
    payload[payloadLen++] = packet[i];
  }

  crc_t crc = crc_init();
  crc = crc_update(crc, payload, payloadLen);
  crc = crc_finalize(crc);

  uint16_t messageCrc = ((uint16_t)packet[7] << 8) | packet[8];
  return (uint16_t)crc == messageCrc;
}

void logGatewayResponse(byte *packet, byte controllerId[3])
{
  if (packet[1] != 0x04 || packet[2] < 0x0B)
  {
    return;
  }

  byte command = packet[10];
  byte result = packet[11];
  const char *resultText = "UNKNOWN";
  if (result == 0x00)
  {
    resultText = "OK";
  }
  else if (result == 0x02)
  {
    resultText = "REJECT";
  }

  Serial.print("[RF] Gateway response ");
  Serial.print(gatewayCommandName(command));
  Serial.print(" ");
  Serial.print(resultText);
  Serial.print(" from ");
  Serial.println(controllerIdToHex(controllerId));

  radioStatusText = "resp ";
  radioStatusText += gatewayCommandName(command);
  radioStatusText += " ";
  radioStatusText += resultText;
}

const char *gatewayCommandName(byte command)
{
  switch (command)
  {
  case 0x01:
    return "ADD";
  case 0x02:
    return "DELETE";
  case 0x03:
    return "RESET";
  case 0x04:
    return "POWER";
  case 0x05:
    return "PING";
  case 0x08:
    return "SWITCH_BIND";
  default:
    return "CMD";
  }
}

void saveCapturedController(byte controllerId[3])
{
  String id = controllerIdToHex(controllerId);
  runtimeControllerIds[pairingTopicIndex] = id;
  saveMappings();

  Serial.print("[Pairing] Bound ");
  Serial.print(lightBindings[pairingTopicIndex].topic);
  Serial.print(" -> ");
  Serial.println(id);

  sendAddGatewayPacket(controllerId);
  stopPairing("captured");
}

void beginWifi()
{
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  Serial.print("[WiFi] Hostname: ");
  Serial.println(deviceHostName);
  WiFi.hostname(deviceHostName.c_str());
  startWifiConnection("boot");
}

void startWifiConnection(const char *reason)
{
  if (configuredWifiSsid.length() == 0)
  {
    Serial.println("[WiFi] SSID is empty; STA connection skipped");
    return;
  }

  WiFi.mode(WIFI_AP_STA);
  lastWifiReconnectAttempt = millis();
  wifiConnectStartedAt = lastWifiReconnectAttempt;
  lastWifiStatus = WiFi.status();

  Serial.print("[WiFi] Connecting to ");
  Serial.print(configuredWifiSsid);
  Serial.print(" (");
  Serial.print(reason);
  Serial.println(")");
  WiFi.begin(configuredWifiSsid.c_str(), configuredWifiPassword.c_str());
}

void maintainWifi()
{
  unsigned long now = millis();
  int status = WiFi.status();

  if (status != lastWifiStatus)
  {
    lastWifiStatus = status;
    Serial.print("[WiFi] Status: ");
    Serial.println(wifiStatusName(status));
  }

  if (status == WL_CONNECTED)
  {
    if (!wifiWasConnected)
    {
      wifiWasConnected = true;
      Serial.print("[WiFi] Connected, IP: ");
      Serial.print(WiFi.localIP());
      Serial.print(", RSSI: ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");
      if (!bemfaAutoSyncDone)
      {
        bemfaAutoSyncDone = true;
        if (syncBemfaDevices() && mqttClient.connected())
        {
          subscribeTopics();
        }
      }
    }
    return;
  }

  if (wifiWasConnected)
  {
    wifiWasConnected = false;
    mqttWasConnected = false;
    if (mqttClient.connected())
    {
      mqttClient.disconnect();
    }
    Serial.println("[WiFi] Lost connection");
  }

  bool connecting = (status == WL_IDLE_STATUS);
  bool timedOut = (wifiConnectStartedAt > 0 && now - wifiConnectStartedAt > WIFI_CONNECT_TIMEOUT_MS);
  bool reconnectDue = (now - lastWifiReconnectAttempt > WIFI_RECONNECT_MS);

  if ((!connecting || timedOut) && reconnectDue)
  {
    WiFi.disconnect(false);
    startWifiConnection(timedOut ? "timeout" : "reconnect");
  }
}

const char *wifiStatusName(int status)
{
  switch (status)
  {
  case WL_IDLE_STATUS:
    return "idle/connecting";
  case WL_NO_SSID_AVAIL:
    return "ssid not found";
  case WL_SCAN_COMPLETED:
    return "scan completed";
  case WL_CONNECTED:
    return "connected";
  case WL_CONNECT_FAILED:
    return "connect failed";
  case WL_CONNECTION_LOST:
    return "connection lost";
  case WL_DISCONNECTED:
    return "disconnected";
  default:
    return "unknown";
  }
}

bool connectMqtt()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    return false;
  }

  Serial.print("[MQTT] Connecting to Bemfa ... ");
  if (!mqttClient.connect(BEMFA_UID))
  {
    Serial.print("failed, state ");
    Serial.println(mqttClient.state());
    mqttWasConnected = false;
    return false;
  }

  Serial.println("connected");
  mqttWasConnected = true;
  subscribeTopics();
  publishGatewayStatus(true);
  return true;
}

void subscribeTopics()
{
  Serial.print("[MQTT] Topic count: ");
  Serial.println(lightBindingCount);

  for (size_t i = 0; i < lightBindingCount; i++)
  {
    String topic = lightBindings[i].topic;
    topic.trim();
    if (topic.length() == 0)
    {
      continue;
    }

    bool ok = mqttClient.subscribe(topic.c_str());
    Serial.print("[MQTT] Subscribe ");
    Serial.print(topic);
    Serial.print(" -> ");
    Serial.println(ok ? "sent" : "failed");
  }
}

void maintainMqtt()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    return;
  }

  if (!mqttClient.connected())
  {
    if (mqttWasConnected)
    {
      mqttWasConnected = false;
      Serial.print("[MQTT] Disconnected, state ");
      Serial.println(mqttClient.state());
    }

    unsigned long now = millis();
    if (now - lastMqttReconnectAttempt > 5000)
    {
      lastMqttReconnectAttempt = now;
      if (connectMqtt())
      {
        lastMqttReconnectAttempt = 0;
      }
    }
  }
  else
  {
    mqttClient.loop();
  }
}

void handleMqtt(char *rawTopic, byte *payload, unsigned int length)
{
  String topic = rawTopic;
  String message;
  message.reserve(length);
  for (unsigned int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }

  message.trim();
  message.toLowerCase();

  Serial.print("[MQTT] ");
  Serial.print(topic);
  Serial.print(" -> ");
  Serial.println(message);

  int index = findBindingByTopic(topic);
  if (index < 0)
  {
    Serial.println("[MQTT] Topic is not configured");
    return;
  }

  bool shouldTurnOn;
  if (message == "off")
  {
    shouldTurnOn = false;
  }
  else if (message == "on")
  {
    shouldTurnOn = true;
  }
  else
  {
    Serial.println("[MQTT] Unsupported command");
    return;
  }

  byte controllerId[3];
  if (!parseControllerId(runtimeControllerIds[index], controllerId))
  {
    Serial.println("[RF] No valid controller ID mapped yet; MQTT command logged only.");
    return;
  }

  startPowerCommand(topic, controllerId, shouldTurnOn);
}

void initBindingsFromConfig()
{
  lightBindingCount = 0;
  for (size_t i = 0; i < staticLightBindingCount; i++)
  {
    String topic = staticLightBindings[i].topic;
    String label = staticLightBindings[i].label;
    topic.trim();
    label.trim();
    if (topic.length() == 0)
    {
      continue;
    }

    addOrUpdateBinding(topic, label);
    int index = findBindingByTopic(topic);
    byte controllerId[3];
    if (index >= 0 && parseControllerId(staticLightBindings[i].controllerId, controllerId))
    {
      runtimeControllerIds[index] = controllerIdToHex(controllerId);
    }
  }
}

bool syncBemfaDevices()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    cloudSyncStatus = "wifi not connected";
    Serial.println("[Bemfa] Sync skipped: WiFi is not connected");
    return false;
  }

  WiFiClient apiClient;
  HTTPClient http;
  String url = "http://apis.bemfa.com/vb/api/v2/groupTopic?openID=";
  url += BEMFA_UID;
  url += "&type=1";

  Serial.println("[Bemfa] Syncing MQTT devices ...");
  if (!http.begin(apiClient, url))
  {
    cloudSyncStatus = "http begin failed";
    Serial.println("[Bemfa] HTTP begin failed");
    return false;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK)
  {
    cloudSyncStatus = "http ";
    cloudSyncStatus += String(httpCode);
    Serial.print("[Bemfa] HTTP failed, code ");
    Serial.println(httpCode);
    http.end();
    return false;
  }

  DynamicJsonDocument filter(512);
  filter["code"] = true;
  filter["data"]["data"][0]["topic"] = true;
  filter["data"]["data"][0]["name"] = true;
  filter["data"]["data"][0]["room"] = true;
  filter["data"]["data"][0]["deviceType"] = true;

  DynamicJsonDocument doc(12288);
  DeserializationError error = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
  http.end();

  if (error)
  {
    cloudSyncStatus = "json ";
    cloudSyncStatus += error.c_str();
    Serial.print("[Bemfa] JSON parse failed: ");
    Serial.println(error.c_str());
    return false;
  }

  int code = doc["code"] | -1;
  if (code != 0)
  {
    cloudSyncStatus = "api code ";
    cloudSyncStatus += String(code);
    Serial.print("[Bemfa] API returned code ");
    Serial.println(code);
    return false;
  }

  JsonArray devices = doc["data"]["data"].as<JsonArray>();
  bool seen[MAX_LIGHT_BINDINGS];
  for (size_t i = 0; i < MAX_LIGHT_BINDINGS; i++)
  {
    seen[i] = false;
  }

  size_t synced = 0;
  for (JsonObject device : devices)
  {
    String topic = device["topic"] | "";
    String name = device["name"] | "";
    String room = device["room"] | "";
    String deviceType = device["deviceType"] | "";
    topic.trim();
    name.trim();
    room.trim();
    deviceType.trim();

    if (!shouldUseBemfaDevice(topic, deviceType))
    {
      continue;
    }

    String label = name.length() ? name : topic;
    if (room.length() && label.indexOf(room) < 0)
    {
      label = room + " " + label;
    }

    int index = findBindingByTopic(topic);
    if (index >= 0)
    {
      lightBindings[index].label = label;
      if (!seen[index])
      {
        synced++;
      }
      seen[index] = true;
      continue;
    }

    if (lightBindingCount >= MAX_LIGHT_BINDINGS)
    {
      cloudSyncStatus = "too many topics";
      Serial.println("[Bemfa] Too many switch topics; increase MAX_LIGHT_BINDINGS");
      break;
    }

    lightBindings[lightBindingCount].topic = topic;
    lightBindings[lightBindingCount].label = label;
    runtimeControllerIds[lightBindingCount] = "";
    seen[lightBindingCount] = true;
    lightBindingCount++;
    synced++;
  }

  size_t writeIndex = 0;
  for (size_t readIndex = 0; readIndex < lightBindingCount; readIndex++)
  {
    if (!seen[readIndex])
    {
      Serial.print("[Bemfa] Removed stale topic ");
      Serial.println(lightBindings[readIndex].topic);
      continue;
    }

    if (writeIndex != readIndex)
    {
      lightBindings[writeIndex] = lightBindings[readIndex];
      runtimeControllerIds[writeIndex] = runtimeControllerIds[readIndex];
    }
    writeIndex++;
  }

  for (size_t i = writeIndex; i < lightBindingCount; i++)
  {
    lightBindings[i].topic = "";
    lightBindings[i].label = "";
    runtimeControllerIds[i] = "";
  }
  lightBindingCount = writeIndex;

  cloudSyncStatus = "synced ";
  cloudSyncStatus += String(synced);
  cloudSyncStatus += " switch topics";
  saveMappings();
  printTopicMappings();
  Serial.print("[Bemfa] ");
  Serial.println(cloudSyncStatus);
  return true;
}

void addOrUpdateBinding(const String &topic, const String &label)
{
  String cleanTopic = topic;
  String cleanLabel = label;
  cleanTopic.trim();
  cleanLabel.trim();
  if (cleanTopic.length() == 0)
  {
    return;
  }

  int existing = findBindingByTopic(cleanTopic);
  if (existing >= 0)
  {
    if (cleanLabel.length())
    {
      lightBindings[existing].label = cleanLabel;
    }
    return;
  }

  if (lightBindingCount >= MAX_LIGHT_BINDINGS)
  {
    cloudSyncStatus = "too many topics";
    Serial.println("[Binding] No free binding slots");
    return;
  }

  lightBindings[lightBindingCount].topic = cleanTopic;
  lightBindings[lightBindingCount].label = cleanLabel.length() ? cleanLabel : cleanTopic;
  runtimeControllerIds[lightBindingCount] = "";
  lightBindingCount++;
}

bool shouldUseBemfaDevice(const String &topic, const String &deviceType)
{
  if (topic.length() == 0)
  {
    return false;
  }
  if (deviceType == "switch")
  {
    return true;
  }
  return topic.endsWith("006");
}

void initWifiConfig()
{
  configuredWifiSsid = WIFI_SSID;
  configuredWifiPassword = WIFI_PASSWORD;
}

void loadMappings()
{
  String data;
  for (int i = 0; i < EEPROM_SIZE; i++)
  {
    byte value = EEPROM.read(i);
    if (value == 0x00 || value == 0xFF)
    {
      break;
    }
    data += (char)value;
  }

  if (!data.startsWith("EL1;"))
  {
    Serial.println("[EEPROM] No saved mappings");
    return;
  }

  int start = 4;
  while (start < data.length())
  {
    int end = data.indexOf(';', start);
    if (end < 0)
    {
      end = data.length();
    }

    String item = data.substring(start, end);
    int sep = item.indexOf('=');
    if (sep > 0)
    {
      String topic = item.substring(0, sep);
      String id = item.substring(sep + 1);

      if (topic == "WIFI")
      {
        int comma = id.indexOf(',');
        if (comma > 0)
        {
          String ssid = hexDecode(id.substring(0, comma));
          String password = hexDecode(id.substring(comma + 1));
          if (ssid.length() > 0)
          {
            configuredWifiSsid = ssid;
            configuredWifiPassword = password;
            Serial.print("[EEPROM] Loaded WiFi SSID: ");
            Serial.println(configuredWifiSsid);
          }
        }
      }
      else if (topic == "DEV")
      {
        int comma = id.indexOf(',');
        if (comma > 0)
        {
          String cachedTopic = hexDecode(id.substring(0, comma));
          String cachedLabel = hexDecode(id.substring(comma + 1));
          addOrUpdateBinding(cachedTopic, cachedLabel);
        }
      }
      else if (topic == "ID")
      {
        int comma = id.indexOf(',');
        if (comma > 0)
        {
          String mappedTopic = hexDecode(id.substring(0, comma));
          String mappedId = id.substring(comma + 1);
          int index = findBindingByTopic(mappedTopic);
          if (index < 0 && mappedTopic.length() > 0)
          {
            addOrUpdateBinding(mappedTopic, mappedTopic);
            index = findBindingByTopic(mappedTopic);
          }

          byte controllerId[3];
          if (index >= 0 && mappedId == "-")
          {
            runtimeControllerIds[index] = "";
          }
          else if (index >= 0 && parseControllerId(mappedId, controllerId))
          {
            runtimeControllerIds[index] = controllerIdToHex(controllerId);
          }
        }
      }
      else
      {
        // Legacy format: topic=controllerId.
        int index = findBindingByTopic(topic);
        byte controllerId[3];
        if (index >= 0 && id == "-")
        {
          runtimeControllerIds[index] = "";
        }
        else if (index >= 0 && parseControllerId(id, controllerId))
        {
          runtimeControllerIds[index] = controllerIdToHex(controllerId);
        }
      }
    }

    start = end + 1;
  }

  Serial.println("[EEPROM] Loaded mappings");
}

void saveMappings()
{
  String data = "EL1;";
  if (configuredWifiSsid.length() > 0)
  {
    data += "WIFI=";
    data += hexEncode(configuredWifiSsid);
    data += ",";
    data += hexEncode(configuredWifiPassword);
    data += ";";
  }

  for (size_t i = 0; i < lightBindingCount; i++)
  {
    String topic = lightBindings[i].topic;
    String label = lightBindings[i].label;
    topic.trim();
    if (topic.length() == 0)
    {
      continue;
    }
    data += "DEV=";
    data += hexEncode(topic);
    data += ",";
    data += hexEncode(label);
    data += ";";
  }

  for (size_t i = 0; i < lightBindingCount; i++)
  {
    String topic = lightBindings[i].topic;
    String id = runtimeControllerIds[i];
    topic.trim();
    id.trim();
    if (topic.length() == 0)
    {
      continue;
    }
    data += "ID=";
    data += hexEncode(topic);
    data += ",";
    data += (id.length() == 6) ? id : "-";
    data += ";";
  }

  if (data.length() >= EEPROM_SIZE)
  {
    Serial.println("[EEPROM] Mapping data too large");
    return;
  }

  for (int i = 0; i < EEPROM_SIZE; i++)
  {
    EEPROM.write(i, 0);
  }
  for (int i = 0; i < data.length(); i++)
  {
    EEPROM.write(i, data[i]);
  }
  EEPROM.commit();
  Serial.println("[EEPROM] Saved mappings");
}

String hexEncode(const String &value)
{
  const char *digits = "0123456789ABCDEF";
  String out;
  out.reserve(value.length() * 2);
  for (size_t i = 0; i < value.length(); i++)
  {
    byte b = (byte)value[i];
    out += digits[(b >> 4) & 0x0F];
    out += digits[b & 0x0F];
  }
  return out;
}

String hexDecode(const String &hex)
{
  String out;
  if (hex.length() % 2 != 0)
  {
    return out;
  }

  out.reserve(hex.length() / 2);
  for (int i = 0; i < hex.length(); i += 2)
  {
    int high = hexNibble(hex[i]);
    int low = hexNibble(hex[i + 1]);
    if (high < 0 || low < 0)
    {
      return "";
    }
    out += (char)((high << 4) | low);
  }
  return out;
}

int hexNibble(char c)
{
  if (c >= '0' && c <= '9')
  {
    return c - '0';
  }
  if (c >= 'A' && c <= 'F')
  {
    return c - 'A' + 10;
  }
  if (c >= 'a' && c <= 'f')
  {
    return c - 'a' + 10;
  }
  return -1;
}

int findBindingByTopic(const String &topic)
{
  for (size_t i = 0; i < lightBindingCount; i++)
  {
    if (topic == lightBindings[i].topic)
    {
      return (int)i;
    }
  }
  return -1;
}

bool parseControllerId(const String &hex, byte controllerId[3])
{
  String id = hex;
  id.trim();
  id.toUpperCase();

  if (id.length() != 6)
  {
    return false;
  }

  for (int i = 0; i < 3; i++)
  {
    char part[3] = {id[i * 2], id[i * 2 + 1], '\0'};
    char *endPtr = nullptr;
    long value = strtol(part, &endPtr, 16);
    if (*endPtr != '\0' || value < 0 || value > 0xFF)
    {
      return false;
    }
    controllerId[i] = (byte)value;
  }

  return true;
}

String controllerIdToHex(byte controllerId[3])
{
  char buffer[7];
  snprintf(buffer, sizeof(buffer), "%02X%02X%02X", controllerId[0], controllerId[1], controllerId[2]);
  return String(buffer);
}

void startPowerCommand(const String &topic, byte controllerId[3], bool powerOn)
{
  Serial.print("[RF] Power request ");
  Serial.print(powerOn ? "ON" : "OFF");
  Serial.print(" for ");
  Serial.println(controllerIdToHex(controllerId));

  cancelPendingPowerAck(controllerId);
  enqueuePowerCommand(topic, controllerId, powerOn, 1, millis());
}

bool sendPowerPacket(byte controllerId[3], bool powerOn)
{
  byte value = powerOn ? 0x01 : 0x00;
  byte packet[] = {
      0x54,
      0x21, 0xA4, 0x23,
      0x03, 0x10,
      0x3A, 0x96, 0x9D,
      0xE7,
      0x00, 0x00,
      0x9B, 0x00,
      controllerId[0], controllerId[1], controllerId[2],
      0x04, 0x02, value};

  const size_t payloadIndexes[] = {4, 5, 6, 7, 8, 12, 13, 14, 15, 16, 17, 18, 19};
  placeCrc(packet, payloadIndexes, sizeof(payloadIndexes) / sizeof(payloadIndexes[0]), 10, 11);

  Serial.print("[RF] Sending ");
  Serial.print(powerOn ? "ON" : "OFF");
  Serial.print(" to ");
  Serial.println(controllerIdToHex(controllerId));

  return transmitPacket(packet, sizeof(packet));
}

bool enqueuePowerCommand(const String &topic, byte controllerId[3], bool powerOn, byte attempt, unsigned long dueAt)
{
  if (attempt == 1)
  {
    bool replacedQueuedCommand = false;
    for (size_t i = 0; i < RF_POWER_QUEUE_SIZE; i++)
    {
      if (powerQueue[i].active && powerQueue[i].topic == topic)
      {
        powerQueue[i].active = false;
        replacedQueuedCommand = true;
      }
    }
    if (replacedQueuedCommand)
    {
      Serial.println("[RF] Replaced queued power command for same topic");
    }
  }

  for (size_t i = 0; i < RF_POWER_QUEUE_SIZE; i++)
  {
    if (!powerQueue[i].active)
    {
      powerQueue[i].active = true;
      powerQueue[i].topic = topic;
      powerQueue[i].controllerId[0] = controllerId[0];
      powerQueue[i].controllerId[1] = controllerId[1];
      powerQueue[i].controllerId[2] = controllerId[2];
      powerQueue[i].powerOn = powerOn;
      powerQueue[i].attempt = attempt;
      powerQueue[i].dueAt = dueAt;
      Serial.print("[RF] Queued power command attempt ");
      Serial.print(attempt);
      Serial.print(" for ");
      Serial.println(controllerIdToHex(controllerId));
      return true;
    }
  }

  Serial.println("[RF] Power queue full; command dropped");
  radioStatusText = "queue full";
  return false;
}

void cancelPendingPowerAck(byte controllerId[3])
{
  for (size_t i = 0; i < POWER_ACK_TRACKER_SIZE; i++)
  {
    if (pendingPowerAcks[i].active && controllerIdsMatch(pendingPowerAcks[i].controllerId, controllerId))
    {
      pendingPowerAcks[i].active = false;
      Serial.print("[RF] Cancelled pending POWER ACK for ");
      Serial.println(controllerIdToHex(controllerId));
    }
  }
}

void trackPowerAck(const String &topic, byte controllerId[3], bool powerOn, byte attempt)
{
  for (size_t i = 0; i < POWER_ACK_TRACKER_SIZE; i++)
  {
    if (pendingPowerAcks[i].active && controllerIdsMatch(pendingPowerAcks[i].controllerId, controllerId))
    {
      pendingPowerAcks[i].active = false;
    }
  }

  for (size_t i = 0; i < POWER_ACK_TRACKER_SIZE; i++)
  {
    if (!pendingPowerAcks[i].active)
    {
      pendingPowerAcks[i].active = true;
      pendingPowerAcks[i].topic = topic;
      pendingPowerAcks[i].controllerId[0] = controllerId[0];
      pendingPowerAcks[i].controllerId[1] = controllerId[1];
      pendingPowerAcks[i].controllerId[2] = controllerId[2];
      pendingPowerAcks[i].powerOn = powerOn;
      pendingPowerAcks[i].attempt = attempt;
      pendingPowerAcks[i].sentAt = millis();
      return;
    }
  }

  Serial.println("[RF] POWER ACK tracker full");
}

bool sendAddGatewayPacket(byte controllerId[3])
{
  return sendGatewayBindingPacket(controllerId, 0x01, "ADD");
}

bool sendDeleteGatewayPacket(byte controllerId[3])
{
  return sendGatewayBindingPacket(controllerId, 0x02, "DELETE");
}

bool sendGatewayBindingPacket(byte controllerId[3], byte command, const char *label)
{
  byte packet[] = {
      0x54,
      0x21, 0xA4, 0x23,
      0x03, 0x0E,
      0x3A, 0x96, 0x9D,
      0x60,
      0x00, 0x00,
      0x9B, 0x00,
      controllerId[0], controllerId[1], controllerId[2],
      command};

  const size_t payloadIndexes[] = {4, 5, 6, 7, 8, 12, 13, 14, 15, 16, 17};
  placeCrc(packet, payloadIndexes, sizeof(payloadIndexes) / sizeof(payloadIndexes[0]), 10, 11);

  Serial.print("[RF] Sending ");
  Serial.print(label);
  Serial.print(" gateway binding to ");
  Serial.println(controllerIdToHex(controllerId));

  return transmitPacket(packet, sizeof(packet));
}

bool configureReceiveMode(bool updateStatus)
{
  if (!radioReady)
  {
    return false;
  }

  int state = radio.fixedPacketLengthMode(PACKET_LENGTH);
  if (state != RADIOLIB_ERR_NONE)
  {
    radioStatusText = "rx length error:" + String(state);
    Serial.print("[RF] Fixed packet mode failed, code ");
    Serial.println(state);
    return false;
  }

  state = radio.setSyncWord(syncWord, sizeof(syncWord));
  if (state != RADIOLIB_ERR_NONE)
  {
    radioStatusText = "rx sync error:" + String(state);
    Serial.print("[RF] RX sync setup failed, code ");
    Serial.println(state);
    return false;
  }

  state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE)
  {
    radioStatusText = "rx error:" + String(state);
    Serial.print("[RF] Start receive failed, code ");
    Serial.println(state);
    return false;
  }

  if (updateStatus)
  {
    radioStatusText = "ready";
  }
  return true;
}

bool transmitPacket(byte *packet, size_t len)
{
  if (!radioReady)
  {
    radioStatusText = "not ready";
    Serial.println("[RF] Radio is not ready");
    return false;
  }

  int state = radio.variablePacketLengthMode();
  if (state != RADIOLIB_ERR_NONE)
  {
    radioStatusText = "tx length error:" + String(state);
    Serial.print("[RF] Variable packet mode failed, code ");
    Serial.println(state);
    configureReceiveMode();
    return false;
  }

  // Match the original transmit-only example. The EasyLight protocol sync
  // bytes are embedded in the payload as 54 21 A4 23.
  state = radio.setSyncWord(0x12, 0xAD);
  if (state != RADIOLIB_ERR_NONE)
  {
    radioStatusText = "tx sync error:" + String(state);
    Serial.print("[RF] TX sync setup failed, code ");
    Serial.println(state);
    configureReceiveMode();
    return false;
  }

  Serial.print("[RF] Packet: ");
  for (size_t i = 0; i < len; i++)
  {
    if (packet[i] < 0x10)
    {
      Serial.print("0");
    }
    Serial.print(packet[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  state = radio.transmit(packet, len);
  if (state != RADIOLIB_ERR_NONE)
  {
    radioStatusText = "tx error:" + String(state);
    Serial.print("[RF] Transmit failed, code ");
    Serial.println(state);
    configureReceiveMode();
    return false;
  }

  Serial.println("[RF] Transmit OK");
  configureReceiveMode();
  return true;
}

void maintainPowerQueue()
{
  unsigned long now = millis();
  if (now < nextRfTxAt)
  {
    return;
  }

  int selected = -1;
  unsigned long selectedDueAt = 0;
  for (size_t i = 0; i < RF_POWER_QUEUE_SIZE; i++)
  {
    if (!powerQueue[i].active || now < powerQueue[i].dueAt)
    {
      continue;
    }

    bool isHigherPriority = selected < 0 ||
                            powerQueue[i].attempt < powerQueue[selected].attempt ||
                            (powerQueue[i].attempt == powerQueue[selected].attempt && powerQueue[i].dueAt < selectedDueAt);
    if (isHigherPriority)
    {
      selected = (int)i;
      selectedDueAt = powerQueue[i].dueAt;
    }
  }

  if (selected < 0)
  {
    return;
  }

  QueuedPowerCommand command = powerQueue[selected];
  powerQueue[selected].active = false;

  Serial.print("[RF] Power attempt ");
  Serial.print(command.attempt);
  Serial.print("/");
  Serial.println(POWER_ACK_MAX_RETRIES + 1);

  receivedFlag = false;
  bool ok = sendPowerPacket(command.controllerId, command.powerOn);
  nextRfTxAt = millis() + RF_TX_GAP_MS;

  if (!ok)
  {
    radioStatusText = "tx failed";
    if (command.attempt <= POWER_ACK_MAX_RETRIES)
    {
      Serial.println("[RF] TX failed; retry queued");
      enqueuePowerCommand(command.topic, command.controllerId, command.powerOn, command.attempt + 1, millis() + POWER_ACK_RETRY_DELAY_MS);
    }
    return;
  }

  publishExpectedState(command.topic, command.powerOn);
  trackPowerAck(command.topic, command.controllerId, command.powerOn, command.attempt);
  radioStatusText = "sent waiting ack";
}

void maintainPowerAcks()
{
  unsigned long now = millis();
  for (size_t i = 0; i < POWER_ACK_TRACKER_SIZE; i++)
  {
    if (!pendingPowerAcks[i].active || now - pendingPowerAcks[i].sentAt <= POWER_ACK_TIMEOUT_MS)
    {
      continue;
    }

    PendingPowerAck ack = pendingPowerAcks[i];
    pendingPowerAcks[i].active = false;

    Serial.print("[RF] No POWER ACK received for ");
    Serial.println(controllerIdToHex(ack.controllerId));
    if (ack.attempt <= POWER_ACK_MAX_RETRIES)
    {
      Serial.println("[RF] Retry queued after missing POWER ACK");
      enqueuePowerCommand(ack.topic, ack.controllerId, ack.powerOn, ack.attempt + 1, now + POWER_ACK_RETRY_DELAY_MS);
      radioStatusText = "retry queued";
    }
    else
    {
      radioStatusText = "sent unacked";
    }
  }
}

void maintainPowerCommand()
{
  maintainPowerAcks();
  maintainPowerQueue();
}

bool handlePowerAck(byte *packet, byte controllerId[3])
{
  if (packet[1] != 0x04 || packet[10] != 0x04)
  {
    return false;
  }

  for (size_t i = 0; i < POWER_ACK_TRACKER_SIZE; i++)
  {
    if (!pendingPowerAcks[i].active || !controllerIdsMatch(pendingPowerAcks[i].controllerId, controllerId))
    {
      continue;
    }

    pendingPowerAcks[i].active = false;
    if (packet[11] == 0x00)
    {
      Serial.println("[RF] POWER ACK OK");
      radioStatusText = "power ack";
      return true;
    }
    if (packet[11] == 0x02)
    {
      Serial.println("[RF] POWER ACK REJECT");
      radioStatusText = "power reject";
      return true;
    }

    Serial.print("[RF] POWER ACK unknown result ");
    Serial.println(packet[11], HEX);
    radioStatusText = "power ack unknown";
    return true;
  }

  return false;
}

bool controllerIdsMatch(byte left[3], byte right[3])
{
  return left[0] == right[0] && left[1] == right[1] && left[2] == right[2];
}

void placeCrc(byte *packet, const size_t *payloadIndexes, size_t payloadLen, size_t crcHighIndex, size_t crcLowIndex)
{
  byte crcPayload[24];
  for (size_t i = 0; i < payloadLen; i++)
  {
    crcPayload[i] = packet[payloadIndexes[i]];
  }

  crc_t crc = crc_init();
  crc = crc_update(crc, crcPayload, payloadLen);
  crc = crc_finalize(crc);

  packet[crcHighIndex] = (byte)(crc >> 8);
  packet[crcLowIndex] = (byte)(crc & 0xFF);
}

void publishExpectedState(const String &topic, bool powerOn)
{
  String stateTopic = topic + "/up";
  const char *state = powerOn ? "on" : "off";

  if (mqttClient.connected())
  {
    mqttClient.publish(stateTopic.c_str(), state, true);
    Serial.print("[MQTT] State updated: ");
    Serial.print(stateTopic);
    Serial.print(" -> ");
    Serial.println(state);
  }
}

String statusTopic()
{
  for (size_t i = 0; i < lightBindingCount; i++)
  {
    String topic = lightBindings[i].topic;
    topic.trim();
    if (topic.length() > 0)
    {
      return topic + "/status/up";
    }
  }
  return "easylight/status/up";
}

void publishGatewayStatus(bool force)
{
  if (!mqttClient.connected())
  {
    return;
  }

  unsigned long now = millis();
  if (!force && now - lastStatusPublishAt < 30000)
  {
    return;
  }
  lastStatusPublishAt = now;

  String payload = "wifi:";
  payload += WiFi.status() == WL_CONNECTED ? "connected" : "disconnected";
  payload += ";mqtt:";
  payload += mqttClient.connected() ? "connected" : "disconnected";
  payload += ";radio:";
  payload += radioStatusText;
  payload += ";ip:";
  payload += WiFi.localIP().toString();
  payload += ";ap:";
  payload += WiFi.softAPIP().toString();

  String topic = statusTopic();
  mqttClient.publish(topic.c_str(), payload.c_str(), true);
  Serial.print("[MQTT] Gateway status: ");
  Serial.print(topic);
  Serial.print(" -> ");
  Serial.println(payload);
}

void printTopicMappings()
{
  Serial.print("[Config] Static topic count: ");
  Serial.println(lightBindingCount);
  for (size_t i = 0; i < lightBindingCount; i++)
  {
    Serial.print("[Config] ");
    Serial.print(lightBindings[i].topic);
    if (lightBindings[i].label.length())
    {
      Serial.print(" (");
      Serial.print(lightBindings[i].label);
      Serial.print(")");
    }
    Serial.print(" -> ");
    Serial.println(runtimeControllerIds[i].length() ? runtimeControllerIds[i] : "(not paired)");
  }
}
