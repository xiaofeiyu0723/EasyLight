/*
  EasyLight Bemfa Gateway

  ESP32 + CC1101 bridge:
    Bemfa/Mijia/Xiaoai switch command -> EasyLight 433 MHz controller packet

  Features:
    - Multi-controller routing by Bemfa topic
    - Long-press pairing button to capture controller IDs
    - Persistent bindings in ESP32 NVS

  Required libraries:
    - PubSubClient
    - RadioLib
    - AceCRC
*/

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <RadioLib.h>
#include <AceCRC.h>
#include <Preferences.h>

#if __has_include(".config/bemfa_gateway_config.h")
#include ".config/bemfa_gateway_config.h"
#else
#error "Create .config/bemfa_gateway_config.h from bemfa_gateway_config.example.h"
#endif

using namespace ace_crc::crc16ccitt_byte;

// Bemfa MQTT
#define BEMFA_MQTT_HOST "bemfa.com"
#define BEMFA_MQTT_PORT 9501

// ESP32 + CC1101 pinout
#define PIN_CS 5
#define PIN_GDO0 21
#define PIN_RST RADIOLIB_NC
#define PIN_GDO2 22

// Pairing button. GPIO0 is the BOOT button on many ESP32 dev boards.
#ifndef PAIR_BUTTON_PIN
#define PAIR_BUTTON_PIN 0
#endif

#ifndef PAIR_HOLD_MS
#define PAIR_HOLD_MS 3000
#endif

#ifndef PAIR_WINDOW_MS
#define PAIR_WINDOW_MS 60000
#endif

// RF configuration
#define RADIO_CARRIER_FREQUENCY 433.3
#define RADIO_BIT_RATE 250.0
#define RADIO_FREQUENCY_DEVIATION 125.0
#define RADIO_RX_BANDWIDTH 270.0
#define RADIO_OUTPUT_POWER 10
#define RADIO_PREAMBLE_LENGTH 32

#define PACKET_LENGTH 24
#define PACKET_THIRD_SYNC_WORD 0x23

#ifndef MAX_LIGHT_BINDINGS
#define MAX_LIGHT_BINDINGS 12
#endif

struct LightBinding
{
  byte controllerId[3];
  String controllerHex;
  String topic;
};

struct StaticLightBinding
{
  const char *topic;
  const char *controllerId;
};

enum GatewayMode
{
  MODE_NORMAL,
  MODE_PAIRING
};

#ifndef STATIC_LIGHT_BINDINGS
#define STATIC_LIGHT_BINDINGS {"", ""}
#endif

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
CC1101 radio = new Module(PIN_CS, PIN_GDO0, PIN_RST, PIN_GDO2);
Preferences preferences;

uint8_t syncWord[] = {0x21, 0xA4};
LightBinding lights[MAX_LIGHT_BINDINGS];
int lightCount = 0;
StaticLightBinding staticLightBindings[] = {STATIC_LIGHT_BINDINGS};

GatewayMode gatewayMode = MODE_NORMAL;
unsigned long pairingStartedAt = 0;
unsigned long buttonPressedAt = 0;
unsigned long lastMqttReconnectAttempt = 0;
bool buttonWasPressed = false;
bool radioReady = false;

volatile bool receivedFlag = false;

#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void setReceivedFlag()
{
  receivedFlag = true;
}

void connectWifi();
bool connectMqtt();
void subscribeLightTopics();
void handleMqtt(char *topic, byte *payload, unsigned int length);
void handlePairButton();
void enterPairingMode();
void exitPairingMode();
void handleRadio();
bool extractControllerId(byte *packet, size_t len, byte controllerId[3]);
bool validateControllerResponse(byte *packet, size_t len);
bool addOrRefreshBinding(byte controllerId[3], bool persist);
void loadBindings();
void saveBindings();
void bootstrapDefaultBinding();
void bootstrapStaticBindings();
int findBindingByTopic(const String &topic);
int findBindingById(byte controllerId[3]);
String controllerIdToHex(byte controllerId[3], bool lowerCase = false);
String topicForController(byte controllerId[3]);
bool parseControllerId(const String &hex, byte controllerId[3]);
bool sendPowerPacket(byte controllerId[3], bool powerOn);
bool sendAddGatewayPacket(byte controllerId[3]);
bool transmitPacket(byte *packet, size_t len);
void placeCrc(byte *packet, size_t *payloadIndexes, size_t payloadLen, size_t crcHighIndex, size_t crcLowIndex);
void publishExpectedState(const String &topic, bool powerOn);

void setup()
{
  Serial.begin(115200);
  delay(100);

  Serial.println();
  Serial.println("[EasyLight] Bemfa multi-light gateway starting");

  pinMode(PAIR_BUTTON_PIN, INPUT_PULLUP);

  preferences.begin("easylight", false);
  loadBindings();
  bootstrapDefaultBinding();
  bootstrapStaticBindings();

  Serial.print("[CC1101] Initializing ... ");
  int state = radio.begin(
      RADIO_CARRIER_FREQUENCY,
      RADIO_BIT_RATE,
      RADIO_FREQUENCY_DEVIATION,
      RADIO_RX_BANDWIDTH,
      RADIO_OUTPUT_POWER,
      RADIO_PREAMBLE_LENGTH);

  radio.setCrcFiltering(false);
  radio.fixedPacketLengthMode(PACKET_LENGTH);
  radio.setSyncWord(syncWord, sizeof(syncWord));
  radio.setPacketReceivedAction(setReceivedFlag);

  if (state == RADIOLIB_ERR_NONE)
  {
    radioReady = true;
    Serial.println("success");
    radio.startReceive();
  }
  else
  {
    Serial.print("failed, code ");
    Serial.println(state);
  }

  connectWifi();

  mqttClient.setServer(BEMFA_MQTT_HOST, BEMFA_MQTT_PORT);
  mqttClient.setCallback(handleMqtt);
  connectMqtt();

  Serial.println("[Pairing] Hold the pair button for 3 seconds to scan controllers.");
}

void loop()
{
  handlePairButton();
  handleRadio();

  if (gatewayMode == MODE_PAIRING && millis() - pairingStartedAt > PAIR_WINDOW_MS)
  {
    exitPairingMode();
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    connectWifi();
  }

  if (!mqttClient.connected())
  {
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

void connectWifi()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    return;
  }

  Serial.print("[WiFi] Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    handlePairButton();
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.print("[WiFi] Connected, IP: ");
  Serial.println(WiFi.localIP());
}

bool connectMqtt()
{
  Serial.print("[MQTT] Connecting to Bemfa ... ");

  if (!mqttClient.connect(BEMFA_UID))
  {
    Serial.print("failed, state ");
    Serial.println(mqttClient.state());
    return false;
  }

  Serial.println("connected");
  subscribeLightTopics();
  return true;
}

void subscribeLightTopics()
{
  Serial.print("[MQTT] Binding count: ");
  Serial.println(lightCount);

  for (int i = 0; i < lightCount; i++)
  {
    bool ok = mqttClient.subscribe(lights[i].topic.c_str());
    Serial.print("[MQTT] Subscribe ");
    Serial.print(lights[i].topic);
    Serial.print(" -> ");
    Serial.println(ok ? "sent" : "failed");
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
    Serial.println("[MQTT] Topic is not bound to a controller");
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

  if (sendPowerPacket(lights[index].controllerId, shouldTurnOn))
  {
    publishExpectedState(lights[index].topic, shouldTurnOn);
  }
}

void handlePairButton()
{
  bool pressed = digitalRead(PAIR_BUTTON_PIN) == LOW;
  unsigned long now = millis();

  if (pressed && !buttonWasPressed)
  {
    buttonPressedAt = now;
  }

  if (pressed && buttonWasPressed && gatewayMode == MODE_NORMAL && now - buttonPressedAt >= PAIR_HOLD_MS)
  {
    enterPairingMode();
  }

  buttonWasPressed = pressed;
}

void enterPairingMode()
{
  gatewayMode = MODE_PAIRING;
  pairingStartedAt = millis();

  Serial.println("[Pairing] Started.");
  Serial.println("[Pairing] Trigger each light/controller now. New controller IDs will be captured and bound.");
  if (radioReady)
  {
    radio.startReceive();
  }
}

void exitPairingMode()
{
  gatewayMode = MODE_NORMAL;
  Serial.println("[Pairing] Stopped.");
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
    Serial.print("[RF] Receive failed, code ");
    Serial.println(state);
    radio.startReceive();
    return;
  }

  if (packet[0] != PACKET_THIRD_SYNC_WORD)
  {
    radio.startReceive();
    return;
  }

  byte controllerId[3];
  if (!extractControllerId(packet, PACKET_LENGTH, controllerId))
  {
    radio.startReceive();
    return;
  }

  String id = controllerIdToHex(controllerId);
  Serial.print("[RF] Controller response from ");
  Serial.println(id);

  if (gatewayMode == MODE_PAIRING)
  {
    bool added = addOrRefreshBinding(controllerId, true);
    if (added)
    {
      saveBindings();
      subscribeLightTopics();
    }

    sendAddGatewayPacket(controllerId);
    Serial.print("[Pairing] Create Bemfa MQTT topic if missing: ");
    Serial.println(topicForController(controllerId));
  }

  radio.startReceive();
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

bool addOrRefreshBinding(byte controllerId[3], bool persist)
{
  int existing = findBindingById(controllerId);
  if (existing >= 0)
  {
    Serial.print("[Binding] Existing ");
    Serial.print(lights[existing].controllerHex);
    Serial.print(" -> ");
    Serial.println(lights[existing].topic);
    return false;
  }

  if (lightCount >= MAX_LIGHT_BINDINGS)
  {
    Serial.println("[Binding] No free binding slots");
    return false;
  }

  LightBinding &binding = lights[lightCount++];
  memcpy(binding.controllerId, controllerId, 3);
  binding.controllerHex = controllerIdToHex(controllerId);
  binding.topic = topicForController(controllerId);

  Serial.print("[Binding] Added ");
  Serial.print(binding.controllerHex);
  Serial.print(" -> ");
  Serial.println(binding.topic);

  if (persist)
  {
    saveBindings();
  }

  return true;
}

void loadBindings()
{
  lightCount = 0;
  String data = preferences.getString("bindings", "");
  if (data.length() == 0)
  {
    Serial.println("[Binding] No saved bindings");
    return;
  }

  int start = 0;
  while (start < data.length() && lightCount < MAX_LIGHT_BINDINGS)
  {
    int end = data.indexOf(';', start);
    if (end < 0)
    {
      end = data.length();
    }

    String item = data.substring(start, end);
    int sep = item.indexOf(':');
    if (sep > 0)
    {
      String idHex = item.substring(0, sep);
      String topic = item.substring(sep + 1);
      byte id[3];
      if (parseControllerId(idHex, id) && topic.length() > 0)
      {
        LightBinding &binding = lights[lightCount++];
        memcpy(binding.controllerId, id, 3);
        binding.controllerHex = controllerIdToHex(id);
        binding.topic = topic;
      }
    }

    start = end + 1;
  }

  Serial.print("[Binding] Loaded ");
  Serial.print(lightCount);
  Serial.println(" binding(s)");
}

void saveBindings()
{
  String data;
  for (int i = 0; i < lightCount; i++)
  {
    data += lights[i].controllerHex;
    data += ":";
    data += lights[i].topic;
    data += ";";
  }

  preferences.putString("bindings", data);
  Serial.println("[Binding] Saved");
}

void bootstrapDefaultBinding()
{
  String idHex = EASYLIGHT_CONTROLLER_ID;
  String topic = BEMFA_TOPIC;
  idHex.trim();
  topic.trim();

  if (idHex.length() == 0 || topic.length() == 0)
  {
    return;
  }

  byte id[3];
  if (!parseControllerId(idHex, id))
  {
    return;
  }

  if (findBindingById(id) >= 0)
  {
    return;
  }

  if (lightCount >= MAX_LIGHT_BINDINGS)
  {
    return;
  }

  LightBinding &binding = lights[lightCount++];
  memcpy(binding.controllerId, id, 3);
  binding.controllerHex = controllerIdToHex(id);
  binding.topic = topic;

  Serial.print("[Binding] Bootstrapped ");
  Serial.print(binding.controllerHex);
  Serial.print(" -> ");
  Serial.println(binding.topic);
}

void bootstrapStaticBindings()
{
  size_t count = sizeof(staticLightBindings) / sizeof(staticLightBindings[0]);
  for (size_t i = 0; i < count; i++)
  {
    String topic = staticLightBindings[i].topic;
    String idHex = staticLightBindings[i].controllerId;
    topic.trim();
    idHex.trim();

    if (topic.length() == 0 || idHex.length() == 0)
    {
      continue;
    }

    byte id[3];
    if (!parseControllerId(idHex, id))
    {
      Serial.print("[Binding] Invalid static controller ID for ");
      Serial.println(topic);
      continue;
    }

    int existingById = findBindingById(id);
    if (existingById >= 0)
    {
      lights[existingById].topic = topic;
      Serial.print("[Binding] Static topic ");
      Serial.print(lights[existingById].controllerHex);
      Serial.print(" -> ");
      Serial.println(topic);
      continue;
    }

    int existingByTopic = findBindingByTopic(topic);
    if (existingByTopic >= 0)
    {
      memcpy(lights[existingByTopic].controllerId, id, 3);
      lights[existingByTopic].controllerHex = controllerIdToHex(id);
      Serial.print("[Binding] Static controller ");
      Serial.print(topic);
      Serial.print(" -> ");
      Serial.println(lights[existingByTopic].controllerHex);
      continue;
    }

    if (lightCount >= MAX_LIGHT_BINDINGS)
    {
      Serial.println("[Binding] No free binding slots for static mapping");
      return;
    }

    LightBinding &binding = lights[lightCount++];
    memcpy(binding.controllerId, id, 3);
    binding.controllerHex = controllerIdToHex(id);
    binding.topic = topic;

    Serial.print("[Binding] Static ");
    Serial.print(binding.controllerHex);
    Serial.print(" -> ");
    Serial.println(binding.topic);
  }
}

int findBindingByTopic(const String &topic)
{
  for (int i = 0; i < lightCount; i++)
  {
    if (lights[i].topic == topic)
    {
      return i;
    }
  }
  return -1;
}

int findBindingById(byte controllerId[3])
{
  for (int i = 0; i < lightCount; i++)
  {
    if (memcmp(lights[i].controllerId, controllerId, 3) == 0)
    {
      return i;
    }
  }
  return -1;
}

String controllerIdToHex(byte controllerId[3], bool lowerCase)
{
  char buffer[7];
  snprintf(buffer, sizeof(buffer), "%02X%02X%02X", controllerId[0], controllerId[1], controllerId[2]);
  String value = buffer;
  if (lowerCase)
  {
    value.toLowerCase();
  }
  return value;
}

String topicForController(byte controllerId[3])
{
  return "el" + controllerIdToHex(controllerId, true) + "006";
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

  size_t payloadIndexes[] = {4, 5, 6, 7, 8, 12, 13, 14, 15, 16, 17, 18, 19};
  placeCrc(packet, payloadIndexes, sizeof(payloadIndexes) / sizeof(payloadIndexes[0]), 10, 11);

  Serial.print("[RF] Sending ");
  Serial.print(powerOn ? "ON" : "OFF");
  Serial.print(" to ");
  Serial.println(controllerIdToHex(controllerId));

  return transmitPacket(packet, sizeof(packet));
}

bool sendAddGatewayPacket(byte controllerId[3])
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
      0x01};

  size_t payloadIndexes[] = {4, 5, 6, 7, 8, 12, 13, 14, 15, 16, 17};
  placeCrc(packet, payloadIndexes, sizeof(payloadIndexes) / sizeof(payloadIndexes[0]), 10, 11);

  Serial.print("[RF] Sending ADD gateway binding to ");
  Serial.println(controllerIdToHex(controllerId));

  return transmitPacket(packet, sizeof(packet));
}

bool transmitPacket(byte *packet, size_t len)
{
  if (!radioReady)
  {
    Serial.println("[RF] Radio is not ready");
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

  int state = radio.transmit(packet, len);
  if (state != RADIOLIB_ERR_NONE)
  {
    Serial.print("[RF] Transmit failed, code ");
    Serial.println(state);
    radio.startReceive();
    return false;
  }

  Serial.println("[RF] Transmit OK");
  radio.startReceive();
  return true;
}

void placeCrc(byte *packet, size_t *payloadIndexes, size_t payloadLen, size_t crcHighIndex, size_t crcLowIndex)
{
  byte payload[24];
  for (size_t i = 0; i < payloadLen; i++)
  {
    payload[i] = packet[payloadIndexes[i]];
  }

  crc_t crc = crc_init();
  crc = crc_update(crc, payload, payloadLen);
  crc = crc_finalize(crc);

  packet[crcHighIndex] = (byte)(crc >> 8);
  packet[crcLowIndex] = (byte)(crc & 0xFF);
}

void publishExpectedState(const String &topic, bool powerOn)
{
  String stateTopic = topic + "/up";
  const char *payload = powerOn ? "on" : "off";

  if (mqttClient.connected())
  {
    mqttClient.publish(stateTopic.c_str(), payload, true);
    Serial.print("[MQTT] State updated: ");
    Serial.print(stateTopic);
    Serial.print(" -> ");
    Serial.println(payload);
  }
}
