#include <WiFi.h>
#include <esp_mac.h>

#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient
#include <cppQueue.h>     // https://github.com/SMFSW/Queue

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

// int parseMQTTTopic(String topic, String topicLevels[]);
int parseMQTTTopic2stack(String topic, cppQueue *topicStack);
int hexToBytes(const String &hex, byte *bytes);

void messageRouter(cppQueue *topicStack, String payload = "");

// #======================== Variables ========================#

WiFiClient espClient;
PubSubClient mqttClient(MQTT_BROKER, MQTT_PORT, mqtt_callback, espClient);
long lastMQTTReconnectAttempt = 0;
String macAddress_s; // MAC Address with out ':'

// A stack of topics
cppQueue topicStack(sizeof(String), ROUTER_MAX_LEVELS, LIFO);

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

    // Payload 2 String
    String payload_s = "";
    for (int i = 0; i < length; i++)
    {
        payload_s += (char)payload[i];
    }
    Serial.println(payload_s);

    // Stack
    int stackSize = parseMQTTTopic2stack(topic, &topicStack);
    // for (int i = 0; i < stackSize; i++)
    // {
    //     String item;
    //     topicStack.pop(&item);
    //     Serial.print("\tStack ");
    //     Serial.print(i);
    //     Serial.print(": ");
    //     Serial.println(item);
    // }

    messageRouter(&topicStack, payload_s);

    // // Levels
    // String topicLevels[ROUTER_MAX_LEVELS];
    // int levelCount = parseMQTTTopic(topic, topicLevels);
    // for (int i = 0; i < levelCount; i++)
    // {
    //     Serial.print("\tLevel ");
    //     Serial.print(i);
    //     Serial.print(": ");
    //     Serial.println(topicLevels[i]);
    // }
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

// int parseMQTTTopic(String topic, String topicLevels[])
// {
//     int levelCount = 0;
//     int lastIndex = 0;

//     for (int i = 0; i < topic.length(); i++)
//     {
//         if (topic[i] == '/')
//         {
//             topicLevels[levelCount] = topic.substring(lastIndex, i);
//             levelCount++;
//             lastIndex = i + 1;
//         }
//     }

//     topicLevels[levelCount] = topic.substring(lastIndex);
//     levelCount++;

//     return levelCount;
// }

int parseMQTTTopic2stack(String topic, cppQueue *topicStack)
{
    int levelCount = 0;
    int lastIndex = topic.length();

    // TODO: Validate the topic

    for (int i = topic.length() - 1; i >= 0; i--)
    {
        if (topic[i] == '/')
        {
            String item = topic.substring(i + 1, lastIndex);
            topicStack->push(&item);
            levelCount++;
            lastIndex = i;
        }
    }

    String item = topic.substring(0, lastIndex);
    topicStack->push(&item);
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

void messageRouter(cppQueue *topicStack, String payload)
{
    // Content
    String bridge_id;
    String controller_id;
    String light_id;

    while (!topicStack->isEmpty())
    {
        String item;
        topicStack->pop(&item);
        Serial.print("Stack Item: ");
        Serial.println(item);

        /*
            LEVEL 1 TO SPECIFIC BRIDGE
         */

        if (item == "easylight")
        {
            if (topicStack->isEmpty())
            {
                Serial.println("\tBridge parameter is missing!");
                return;
            }
            topicStack->pop(&bridge_id);
            Serial.print("\tRouter to Bridge ID: "); // TO NEXT LEVEL
            Serial.println(bridge_id);
        }

        /*
            LEVEL 2 CONTROLLERS ACTION or TO SPECIFIC CONTROLLER
        */

        else if (item == "controller" && bridge_id.length() == 12)
        {
            if (topicStack->isEmpty())
            {
                Serial.println("\t\tController parameter is missing!");
                return;
            }

            String controller_op;
            topicStack->pop(&controller_op);

            if (controller_op == "add")
            {
                Serial.print("\t\t[Bridge adding] controller: "); // TAKE ACTION
                Serial.println(payload);
                return;
            }
            else if (controller_op == "remove")
            {
                Serial.print("\t\t[Bridge removing] controller: "); // TAKE ACTION
                Serial.println(payload);
                return;
            }
            else if (controller_op.length() == 6)
            {
                Serial.print("\t\tRouter to Controller ID: "); // TO NEXT LEVEL
                Serial.println(controller_op);
                controller_id = controller_op;
            }
            else
            {
                Serial.print("\t\tInvalid Controller Operation: ");
                Serial.println(controller_op);
                return;
            }
        }

        /*
            LEVEL 3 SPECIFIC CONTROLLER ACTION or TO SPECIFIC LIGHT
        */

        else if (item == "reset" && bridge_id.length() == 12 && controller_id.length() == 6)
        {
            Serial.print("\t\t\t[Resetting] Controller: "); // TAKE ACTION
            Serial.println(payload);
            return;
        }

        else if (item == "light" && bridge_id.length() == 12 && controller_id.length() == 6)
        {
            if (topicStack->isEmpty())
            {
                Serial.println("\t\t\tLight parameter is missing!");
                return;
            }
            topicStack->pop(&light_id);
            Serial.print("\t\t\tRouter to Light ID: "); // TO NEXT LEVEL
            Serial.println(light_id);
        }

        /*
            LEVEL 4 SPECIFIC LIGHT ACTION
        */

        else if (item == "power" && bridge_id.length() == 12 && controller_id.length() == 6 && light_id.length() == 1)
        {
            if (topicStack->isEmpty())
            {
                Serial.println("\t\t\t\tPower parameter is missing!");
                return;
            }

            String power_op;
            topicStack->pop(&power_op);

            if (power_op == "set")
            {
                Serial.print("\t\t\t\t[Setting] Power: "); // TAKE ACTION
                Serial.println(payload);
                return;
            }
            else
            {
                Serial.print("\t\t\t\tInvalid Power Operation: ");
                Serial.println(power_op);
                return;
            }
        }

        else if (item == "switch" && bridge_id.length() == 12 && controller_id.length() == 6 && light_id.length() == 1)
        {
            if (topicStack->isEmpty())
            {
                Serial.println("\t\t\t\tSwitch parameter is missing!");
                return;
            }

            String switch_op;
            topicStack->pop(&switch_op);

            if (switch_op == "add")
            {
                Serial.print("\t\t\t\t[Adding] Switch: "); // TAKE ACTION
                Serial.println(payload);
                return;
            }
            else if (switch_op == "remove")
            {
                Serial.print("\t\t\t\t[Removing] Switch: "); // TAKE ACTION
                Serial.println(payload);
                return;
            }
            else
            {
                Serial.print("\t\t\t\tInvalid Switch Operation: ");
                Serial.println(switch_op);
                return;
            }
        }

        // ERROR

        else
        {
            Serial.print("Invalid Item: ");
            Serial.println(item);
        }
    }
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