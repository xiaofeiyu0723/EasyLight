#include <Arduino.h>

#include <cppQueue.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <RadioLib.h>
#include <AceCRC.h>

/*
    Move Arduino code to PlatformIO
    ToDo List:
    - Support MQTT 5.0 (for no local)
    - Decouple different modules
    - Maybe use FreeRTOS
    - Receive and Parse the response from controller
*/

void setup()
{
    Serial.begin(9600);
}

void loop()
{
    Serial.println("Hello PIO1 world!");
    delay(1000);
}