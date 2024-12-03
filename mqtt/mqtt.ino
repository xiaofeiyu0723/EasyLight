#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_mac.h>

#include ".config/mqtt_config.h" // MQTT & WIFI Configurations (Private)

// #======================== Definitions ========================#

/*
    Please Create a file named "mqtt_config.h" in the ".config" folder.
    Then configure the following parameters in the file.

        // WiFi Configurations
        #define WIFI_SSID "YOUR_WIFI_SSID"
        #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

        // MQTT Configurations
        #define MQTT_BROKER "YOUR_MQTT_BROKER"
        #define MQTT_PORT 1883
        #define MQTT_CLIENT_ID "YOUR_CLIENT_ID"
        #define MQTT_USERNAME "YOUR_USERNAME"
        #define MQTT_PASSWORD "YOUR_PASSWORD"

*/

#define MQTT_RECONNECT_DELAY 5000
#define ROUTER_MAX_LEVELS 10

// #======================== Prototypes ========================#

void mqtt_callback(char *topic, byte *payload, unsigned int length);
boolean mqtt_reconnect();

int parseMQTTTopic(String topic, String topicLevels[]);
int hexToBytes(const String &hex, byte *bytes);

// #======================== Variables ========================#

WiFiClient espClient;
PubSubClient mqttClient(MQTT_BROKER, MQTT_PORT, mqtt_callback, espClient);
long lastMQTTReconnectAttempt = 0;
String macAddress_s; // MAC Address with out ':'

// #======================== Initialization ========================#

void setup()
{
    Serial.begin(115200);
    delay(10);

    Serial.print("[WiFi] Connecting to ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Serial.print("[WiFi] MAC Address: ");
    // Serial.println(WiFi.macAddress());
    macAddress_s = WiFi.macAddress();
    macAddress_s.replace(":", "");
    Serial.print("[WiFi] Short MAC Address: ");
    Serial.println(macAddress_s);

    // Will try for about 10 seconds (20x 500ms)
    int tryDelay = 500;
    int numberOfTries = 20;

    // Wait for the WiFi event
    while (true)
    {

        switch (WiFi.status())
        {
        case WL_NO_SSID_AVAIL:
            Serial.println("[WiFi] SSID not found");
            break;
        case WL_CONNECT_FAILED:
            Serial.print("[WiFi] Failed - WiFi not connected! Reason: ");
            return;
            break;
        case WL_CONNECTION_LOST:
            Serial.println("[WiFi] Connection was lost");
            break;
        case WL_SCAN_COMPLETED:
            Serial.println("[WiFi] Scan is completed");
            break;
        case WL_DISCONNECTED:
            Serial.println("[WiFi] WiFi is disconnected");
            break;
        case WL_CONNECTED:
            Serial.println("[WiFi] WiFi is connected!");
            Serial.print("[WiFi] IP address: ");
            Serial.println(WiFi.localIP());
            return;
            break;
        default:
            Serial.print("[WiFi] WiFi Status: ");
            Serial.println(WiFi.status());
            break;
        }
        delay(tryDelay);

        if (numberOfTries <= 0)
        {
            Serial.print("[WiFi] Failed to connect to WiFi!");
            // Use disconnect function to force stop trying to connect
            WiFi.disconnect();
            return;
        }
        else
        {
            numberOfTries--;
        }
    }
}

// #======================== Main Loop ========================#

void loop()
{
    if (!mqttClient.connected())
    {
        long now = millis();
        if (now - lastMQTTReconnectAttempt > MQTT_RECONNECT_DELAY)
        {
            lastMQTTReconnectAttempt = now;
            if (mqtt_reconnect())
            {
                lastMQTTReconnectAttempt = 0;
            }
        }
    }
    else
    {
        mqttClient.loop();
    }
}

// #======================== Functions ========================#
void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("[MQTT] Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    // Levels
    String topicLevels[ROUTER_MAX_LEVELS];
    int levelCount = parseMQTTTopic(topic, topicLevels);
    for (int i = 0; i < levelCount; i++)
    {
        Serial.print("\tLevel ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(topicLevels[i]);
    }
}

boolean mqtt_reconnect()
{
    Serial.print("[MQTT] Connecting to ");
    Serial.println(MQTT_BROKER);

    String clientId = "EL_" + macAddress_s; // Unique Client ID

    if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD))
    {
        Serial.println("[MQTT] Connected!");
        String topic = "easylight/" + macAddress_s + "/#";
        mqttClient.subscribe(topic.c_str());
        Serial.print("[MQTT] Subscribed to ");
        Serial.println(topic);
    }
    else
    {
        Serial.print("[MQTT] Failed to connect to MQTT Broker! Reason: ");
        Serial.println(mqttClient.state());
    }
    return mqttClient.connected();
}

int parseMQTTTopic(String topic, String topicLevels[])
{
    int levelCount = 0;
    int lastIndex = 0;

    for (int i = 0; i < topic.length(); i++)
    {
        if (topic[i] == '/')
        {
            topicLevels[levelCount] = topic.substring(lastIndex, i);
            levelCount++;
            lastIndex = i + 1;
        }
    }

    topicLevels[levelCount] = topic.substring(lastIndex);
    levelCount++;

    return levelCount;
}

int hexToBytes(const String &hex, byte *bytes)
{
    int len = hex.length() / 2;
    for (int i = 0; i < len; i++)
    {
        bytes[i] = (hex[i * 2] > '9' ? hex[i * 2] - 'A' + 10 : hex[i * 2] - '0') << 4 |
                   (hex[i * 2 + 1] > '9' ? hex[i * 2 + 1] - 'A' + 10 : hex[i * 2 + 1] - '0');
    }
    return len;
}

/*
    easylight/C049EFCC9FFF/controller/add                             -> CONTROLLER_ADD    4
    easylight/C049EFCC9FFF/controller/remove                          -> CONTROLLER_REMOVE 4
    easylight/C049EFCC9FFF/controller/36F9FF/reset                    -> CONTROLLER_RESET  4
    // easylight/C049EFCC9FFF/controller/36F9FF/state
    // easylight/C049EFCC9FFF/controller/36F9FF/light/1/power
    easylight/C049EFCC9FFF/controller/36F9FF/light/1/power/set        -> LIGHT_POWER_SET   8
    easylight/C049EFCC9FFF/controller/36F9FF/switch/add               -> SWITCH_ADD        6
    easylight/C049EFCC9FFF/controller/36F9FF/switch/remove            -> SWITCH_REMOVE     6

*/

/*
// Possible values for client.state()
#define MQTT_CONNECTION_TIMEOUT     -4
#define MQTT_CONNECTION_LOST        -3
#define MQTT_CONNECT_FAILED         -2
#define MQTT_DISCONNECTED           -1
#define MQTT_CONNECTED               0
#define MQTT_CONNECT_BAD_PROTOCOL    1
#define MQTT_CONNECT_BAD_CLIENT_ID   2
#define MQTT_CONNECT_UNAVAILABLE     3
#define MQTT_CONNECT_BAD_CREDENTIALS 4
#define MQTT_CONNECT_UNAUTHORIZED    5
*/