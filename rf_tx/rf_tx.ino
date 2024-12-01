/*
  A Kinetic energy switch Signal Transmitter (Basic)
  Use the CC1101 module
  Imitate the signal of a kinetic energy switch (Just toggle the light)

  Author:  Xiaofei YU, Lan HUANG
  Date:    Nov 2024
*/

#include <Arduino.h>
#include <RadioLib.h>
#include <AceCRC.h>

/*
  CC1101 Connection
    SPI pins:
    - CSn  -> CS or H-CS    = GPIO15
    - SCK  -> SCK/CLK/H-SCK = GPIO14
    - MISO -> MISO/H-MISO   = GPIO12
    - MOSI -> MOSI/H-MOSI   = GPIO13
    IO(interupt) pins:
    - GDO0 -> D1            = GPIO5
    - GDO2 -> D2            = GPIO4

  Gateway Packet Structure

    ADD(14 Bytes):
      Preamble(X）  like  01010101...0000100001 = 0X55, 0x55, ... 0x55, 0x54 (TODO: TBD)
      Sync Word(3)  like  0x21, 0xA4, (0x23)
      -------------------------------------
      DeviceType(1) like  0x03
      Length(1)     like  0x0E
      TBD(3)        like  0x3A, 0x96, 0x9D
      *Operation(1)  like  0x60
      *CRC(2)        like  0x42, 0xBE
      TBD(2)        like  0x9B, 0x00
      Controller(3) like 0x36, 0xF9, 0x8D
      Command(1)    like 0x01
      -------------------------------------
      The Segment with * is not included in the CRC calculation

    ON(16 Bytes):
      Preamble(X）  like  01010101...0000100001 = 0X55, 0x55, ... 0x55, 0x54 (TODO: TBD)
      Sync Word(3)  like  0x21, 0xA4, (0x23)
      -------------------------------------
      DeviceType(1) like  0x03
      Length(1)     like  0x10
      TBD(3)        like  0x3A, 0x96, 0x9D
      *Operation(1)  like  0xE7
      *CRC(2)        like  0x97, 0xCE
      TBD(2)        like  0x9B, 0x00
      Controller(3) like 0x36, 0xF9, 0x8D
      Command(1)    like 0x04
      Parameter(2)  like 0x02
      Value(1)      like 0x01
      -------------------------------------
      The Segment with * is not included in the CRC calculation

*/

// #======================== Definitions ========================#

// Pinout

// For ESP8266
#define PIN_CS 15
#define PIN_GDO0 5
#define PIN_RST RADIOLIB_NC
#define PIN_GDO2 4

/*
// For ESP32
#define PIN_CS 5
#define PIN_GDO0 21
#define PIN_RST RADIOLIB_NC
#define PIN_GDO2 22
*/

/*
// For ESP32-C3
#define PIN_CS 7
#define PIN_GDO0 3
#define PIN_RST RADIOLIB_NC
#define PIN_GDO2 2
*/

// Pinout end

// Serial
#define SERIAL_BAUD 9600

// Radio
#define RADIO_CARRIER_FREQUENCY 433.3   // MHz
#define RADIO_BIT_RATE 250.0            // kbps
#define RADIO_FREQUENCY_DEVIATION 125.0 // kHz
#define RADIO_RX_BANDWIDTH 270.0        // kHz
#define RADIO_OUTPUT_POWER 10           // dBm
#define RADIO_PREAMBLE_LENGTH 32        // bits

// Packet
using namespace ace_crc::crc16ccitt_byte; // Select the type of CRC algorithm we'll be using

// Special
volatile bool transmittedFlag = true; // flag to indicate that a packet was sent
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void setFlag(void)
{
  transmittedFlag = true; // we sent a packet, set the flag
}

// #======================== Variables ========================#

CC1101 radio = new Module(PIN_CS, PIN_GDO0, PIN_RST, PIN_GDO2);

struct LightPacket {
    byte preamble[2];
    byte syncWord[3];
    byte deviceType;
    byte length;
    byte fixedTBD_0[3];
    byte operation;
    byte crc[2];
    byte fixedTBD_1[2];
    byte controllerID[3];
    byte command;
    byte parameter;
    byte status;
};

class Light {
private:
    LightPacket packet;

    int sendPacket() {
        int packetSize = (int)packet.length;
        int totalPacketSize = (int)packet.length +5;
        int payloadSize = packetSize - 3;

        byte payload[] = {
            packet.deviceType, packet.length, 
            packet.fixedTBD_0[0], packet.fixedTBD_0[1], packet.fixedTBD_0[2],
            packet.fixedTBD_1[0], packet.fixedTBD_1[1], 
            packet.controllerID[0], packet.controllerID[1], packet.controllerID[2], 
            packet.command, packet.parameter, packet.status
        };
        crc_t crc = crc_init();
        crc = crc_update(crc, payload, payloadSize);
        crc = crc_finalize(crc);
        packet.crc[0] = crc >> 8;
        packet.crc[1] = crc & 0xFF;

        byte *bytePtr = (byte*)&packet;
        for (int i = 0; i < totalPacketSize; i++) {
            Serial.printf("%02X ", bytePtr[i]);
        }
        Serial.println();
        int state = radio.startTransmit(bytePtr, totalPacketSize);
        return state;
    }

public:
    Light(byte controllerID[3]) {
        memset(&packet, 0, sizeof(packet));
        packet.preamble[0] = 0xAA;
        packet.preamble[1] = 0x54;

        packet.syncWord[0] = 0x21;
        packet.syncWord[1] = 0xA4;
        packet.syncWord[2] = 0x23;

        packet.deviceType = 0x03;

        packet.length = 0x00;

        packet.fixedTBD_0[0] = 0x3A;
        packet.fixedTBD_0[1] = 0x96;
        packet.fixedTBD_0[2] = 0x9D;

        packet.operation = 0xE7;

        packet.crc[0] = 0x00;
        packet.crc[1] = 0x00;
        packet.crc[2] = 0x00;

        packet.fixedTBD_1[0] = 0x9B;
        packet.fixedTBD_1[1] = 0x00;

        memcpy(packet.controllerID, controllerID, 3);

        packet.command = 0x00;
        packet.parameter = 0x00;
        packet.status = 0x00;
    }

    void turnOn() {
        packet.length = 0x10;
        packet.operation = 0xE7;
        packet.command = 0x04;
        packet.parameter = 0x02;
        packet.status = 0x01;
        sendPacket();
    }

    void turnOff() {
        packet.length = 0x10;
        packet.operation = 0xE7;
        packet.command = 0x04;
        packet.parameter = 0x02;
        packet.status = 0x00;
        sendPacket();
    }

    void add() {
        packet.length = 0x0E;
        packet.operation = 0x60;
        packet.command = 0x01;
        sendPacket();
    }

    void del() {
        packet.length = 0x0E;
        packet.operation = 0x60;
        packet.command = 0x02;
        sendPacket();
    }

    void reset() {
        packet.length = 0x0E;
        packet.operation = 0x60;
        packet.command = 0x03;
        sendPacket();
    }

    void ping() {
        packet.length = 0x0F;
        packet.operation = 0x2A;
        packet.command = 0x05;
        packet.parameter = 0x00;
        sendPacket();
    }

    void bind() {
        packet.length = 0x0F;
        packet.operation = 0x2A;
        packet.command = 0x08;
        packet.parameter = 0x01;
        sendPacket();
    }
    void unbind() {
        packet.length = 0x0F;
        packet.operation = 0x2A;
        packet.command = 0x08;
        packet.parameter = 0x02;
        sendPacket();
    }

};

byte controllerID[] = {0x36, 0xAF, 0x6C};
Light light_test(controllerID);
// #======================== Initialization ========================#

void setup()
{
  Serial.begin(SERIAL_BAUD);

  Serial.print(F("[CC1101] Initializing ... "));
  int state = radio.begin(RADIO_CARRIER_FREQUENCY, RADIO_BIT_RATE, RADIO_FREQUENCY_DEVIATION, RADIO_RX_BANDWIDTH, RADIO_OUTPUT_POWER, RADIO_PREAMBLE_LENGTH);
  // Check Init
  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true)
    {
      delay(10);
    }
  }

  radio.setPacketSentAction(setFlag); // Callback on packet sent
}

// #======================== Main Loop ========================#

void loop()
{
  if (transmittedFlag)
  {
    radio.finishTransmit();  // Clear Transmitor
    transmittedFlag = false; // flag to indicate that a packet is being sent
    // ADD
    Serial.println("===ADD===");
    light_test.add();
    delay(5000);
    light_test.turnOn();
    delay(5000);
    light_test.turnOff();
    delay(5000);
    light_test.ping();
    delay(5000);
    // DEL
    Serial.println("===DEL===");
    light_test.del();
    delay(5000);
    light_test.turnOn();
    delay(5000);
    light_test.turnOff();
    delay(5000);
    light_test.ping();
    delay(5000);

    // light_test.bind();
    // delay(5000);
    // light_test.unbind();
    // delay(5000);
    // light_test.reset();
    // delay(5000);
  }
}
