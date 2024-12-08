#include <WiFiClientSecure.h>
#include <MQTT.h>

#include "el_radio.h"     // Radio Module
#include "el_wifi.h"      // WiFI Client
#include "el_mqtt.h"      // MQTT Client
#include "el_protocol.h"  // Protocol(Light)
#include "el_service.h"   // Service(App)

// #======================== Definitions ========================#

#define SERIAL_BAUD 115200

// #======================== Global Variables ========================#

// #======================== Prototypes ========================#

void cb_mqttMessageReceived(String &topic, String &payload);
void cb_radioMessageReceived(uint8_t *packet, size_t len);

// #======================== Initialization ========================#

void setup()
{
  Serial.begin(SERIAL_BAUD);

  Serial.println(" #====== EasyLight Initializing ======#");

  radio_setLoggerOutput(&Serial);
  radio_init();
  radio_setReceiveCallback(cb_radioMessageReceived);
  
  wifi_setLoggerOutput(&Serial);
  wifi_init();
  
  mqtt_setLoggerOutput(&Serial);
  mqtt_init();
  mqtt_setCallback(cb_mqttMessageReceived);

  protocol_setLoggerOutput(&Serial);
  protocol_init();

  Serial.println(" #======== EasyLight Starting ========#");
  
  wifi_connect_blocking();
  mqtt_connect_blocking();

  Serial.println(" #========== EasyLight Ready =========#");
}

// #======================== Main Loop ========================#

void loop()
{
  radio_handle();
  wifi_handle();
  mqtt_handle();
  protocol_handle();
  // delay(10); // <- fixes some issues with WiFi stability
}

// #======================== Functions ========================#

String byte2HexStr_s(byte *byteArr, size_t len)
{
  char hexStr[len * 3 - 1];
  for (size_t i = 0; i < len - 1; i++)
  {
    sprintf(hexStr + i * 3, "%02X ", byteArr[i]);
  }
  sprintf(hexStr + (len - 1) * 3, "%02X", byteArr[len - 1]);
  return String(hexStr);
}

String byte2HexStr(byte *byteArr, size_t len)
{
  char hexStr[len * 2];
  for (size_t i = 0; i < len; i++)
  {
    sprintf(hexStr + i * 2, "%02X", byteArr[i]);
  }
  return String(hexStr);
}

// #======================== Callbacks ========================#

void cb_mqttMessageReceived(String &topic, String &payload)
{
  Serial.println("MQTT Receive: " + topic + " - " + payload);
  // Note: Do not use the MQTT_Client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `MQTT_Client.loop()`.

  // TODO: Handle MQTT messages
}

void cb_radioMessageReceived(uint8_t *packet, size_t len)
{
  // Only the packet passed the CRC check will be received here

  Serial.print("Radio Receive: ");
  for (size_t i = 0; i < len; i++)
  {
    Serial.printf("%02X ", packet[i]);
  }
  Serial.println();

  // TODO: Handle Radio messages
  byte msgType = packet[0];
  if(msgType == 0x01 || msgType == 0x02 || msgType == 0x04)
  {
    if(msgType == 0x01)
    {
      //01 0C 00 AA BB C6 A3 86 CC 01 01 41
      Serial.println("Controller Response");
      byte controllerId[3] = {packet[3], packet[4], packet[8]};
      String controllerIdHex = byte2HexStr(controllerId, 3);
      String controllerReqCode = byte2HexStr(packet + 9, 1);
      String controllerResValue = byte2HexStr(packet + 10, 1);
      String controllerType = byte2HexStr(packet + 11, 1);
      Serial.print("\tController ID: ");
      Serial.println(controllerIdHex);
      Serial.print("\tRequest Code: ");
      Serial.println(controllerReqCode);
      Serial.print("\tResponse Value: ");
      Serial.println(controllerResValue);
      Serial.print("\tController Type: ");
      Serial.println(controllerType);

      // Publish to MQTT
      mqtt_publish_light_state(controllerIdHex, controllerReqCode, controllerResValue);
    }
    else if(msgType == 0x02)
    {
      Serial.println("002 TBD");
    }
    else if(msgType == 0x04)
    {
      Serial.println("002 TBD");
    }


  }
  else if (len == 7)
  {
    // Maybe Switch
    Serial.println("Switch Signal");
    String switchIdHex = byte2HexStr(packet, 4);
    String switchKeyHex = byte2HexStr(packet + 4, 1);
    Serial.print("\tSwitch ID: ");
    Serial.println(switchIdHex);
    Serial.print("\tSwitch key: ");
    Serial.println(switchKeyHex);
  }

}

// #======================== Interrupt ========================#

// #======================== End ========================#