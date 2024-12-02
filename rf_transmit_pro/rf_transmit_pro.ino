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

// Pinout ========================
/* 
  @@@ Caution @@@ 
    Please check the pinout of your board，otherwise it may lead to the flash destroyed !!!
    If it happens, you force to flash the bootloader to the board.
    (Press and hold the BOOT button, then POWER ON again to enter burning mode)
    Then, in the Arduino IDE, select the correct board and port, and burn the bootloader.
*/


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

// Pinout end ========================

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

// #======================== Prototypes ========================#

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

    // =#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#

    byte controllerID[3] = {0x36, 0xF9, 0x8D};
    Serial.print(F("[CC1101] Sending packet ... "));
    Serial.print(F("controllerID = 0x"));
    for (int i = 0; i < 3; i++)
    {
      Serial.print(controllerID[i], HEX);
    }

    // =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+= ON/OFF (3 params) =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
    
    /*

    // A Minimal Packet (OPEN=E7)
    byte byteArr[] = {0x54, 0x21, 0xA4, 0x23, 0x03, 0x10, 0x3A, 0x96, 0x9D, 0xE7, 0x00, 0x00, 0x9B, 0x00, controllerID[0], controllerID[1], controllerID[2], 0x04, 0x02, 0x01};

    // place the CRC at the middle of the packet
    byte payload[] = {byteArr[4], byteArr[5], byteArr[6], byteArr[7], byteArr[8], byteArr[12], byteArr[13], byteArr[14], byteArr[15], byteArr[16], byteArr[17], byteArr[18], byteArr[19]};
    crc_t crc = crc_init();
    crc = crc_update(crc, payload, 13);
    crc = crc_finalize(crc);
    byteArr[10] = (byte)(crc >> 8);
    byteArr[11] = (byte)(crc & 0xFF);

    // send the packet
    int state = radio.startTransmit(byteArr, 20);

    */

    // =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+= ADD/DEL (1 param) =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=

    /*

    // A Minimal Packet (ADD=60)
    byte byteArr[] = {0x54, 0x21, 0xA4, 0x23, 0x03, 0x0E, 0x3A, 0x96, 0x9D, 0x60, 0x00, 0x00, 0x9B, 0x00, controllerID[0], controllerID[1], controllerID[2], 0x01};

    // place the CRC at the middle of the packet
    byte payload[] = {byteArr[4], byteArr[5], byteArr[6], byteArr[7], byteArr[8], byteArr[12], byteArr[13], byteArr[14], byteArr[15], byteArr[16], byteArr[17]};
    crc_t crc = crc_init();
    crc = crc_update(crc, payload, 11);
    crc = crc_finalize(crc);
    byteArr[10] = (byte)(crc >> 8);
    byteArr[11] = (byte)(crc & 0xFF);

    // send the packet
    int state = radio.startTransmit(byteArr, 18);

    */

    // =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+= PING (2 params) =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=

    

    // A Minimal Packet (PING=2A)
    byte byteArr[] = {0x54, 0x21, 0xA4, 0x23, 0x03, 0x0F, 0x3A, 0x96, 0x9D, 0x2A, 0x00, 0x00, 0x9B, 0x00, controllerID[0], controllerID[1], controllerID[2], 0x05, 0x00};

    // place the CRC at the middle of the packet
    byte payload[] = {byteArr[4], byteArr[5], byteArr[6], byteArr[7], byteArr[8], byteArr[12], byteArr[13], byteArr[14], byteArr[15], byteArr[16], byteArr[17], byteArr[18]};
    crc_t crc = crc_init();
    crc = crc_update(crc, payload, 12);
    crc = crc_finalize(crc);
    byteArr[10] = (byte)(crc >> 8);
    byteArr[11] = (byte)(crc & 0xFF);

    // send the packet
    int state = radio.startTransmit(byteArr, 19);

    

    // =#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#

    if (state == RADIOLIB_ERR_NONE)
    {
      Serial.println(F("transmission finished!"));
    }
    else
    {
      Serial.print(F("failed, code "));
      Serial.println(state);
    }

    delay(10000);
  }
}

/*
  Test Packet

  // ################################################

  // ADD
  byte byteArr[] = {0xFF,0x55,0x55,0x55,0x55,0x54,0x21,0xA4,0x23,    0x03,0x0E,   0x3A,0x96,0x9D,    0x60,    0x42,0xBE,    0x9B,0x00,   0x36,0xF9,0x8D,   0x01};
  // Meaning                                                         Getw,Len     TBD fixed          op       CRC           TBD fixed    Controller ID     Cmd
  // Segment used for calculating CRC                                _________    ______________                            —————————    ——————————————    —————
  // 03 0E 3A 96 9D 9B 00 36 F9 8D 01
  int state = radio.startTransmit(byteArr, 23);

  // DEL
  byte byteArr[] = {0xFF,0x55,0x55,0x55,0x55,0x54,0x21,0xA4,0x23,    0x03,0x0E,   0x3A,0x96,0x9D,    0x60,    0xBD,0x30,    0x9B,0x00,   0x36,0xAF,0x6C,   0x02};
  // 03 0E 3A 96 9D 9B 00 36 Af 6C 02
  int state = radio.startTransmit(byteArr, 23);


  // ################################################

  // ON
  byte byteArr2[] = {0xFF,0x55,0x55,0x55,0x55,0x54,0x21,0xA4,0x23,  0x03,0x10,   0x3A,0x96,0x9D,    0xE7,    0x97,0xCE,    0x9B,0x00,   0x36,0xF9,0x8D,   0x04,0x02,   0x01};
  // 03 10 3A 96 9D 9B 00 36 F9 8D 04 02 01
  int state2 = radio.startTransmit(byteArr2, 25);

  // OFF
  // byte byteArr[] = {0xFF,0x55,0x55,0x55,0x55,0x54,0x21,0xA4,0x23,  0x03,0x10,   0x3A,0x96,0x9D,    0xE7,    0xAD,0x06,    0x9B,0x00,   0x36,0xAF,0x6C,   0x04,0x02,   0x00};

  // ################################################

  // ping
  // byte byteArr[] = {0xFF,0x55,0x55,0x55,0x55,0x54,0x21,0xA4,0x23,    0x03,0x0F,   0x3A,0x96,0x9D,    0x2A,    0x07,0xA8,    0x9B,0x00,   0x36,0xAF,0x6C,   0x05,0x00};
  // int state = radio.startTransmit(byteArr, 24);

*/