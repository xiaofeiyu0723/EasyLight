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

  Switch Packet Structure
    Preamble(X） like  01010101...0000100001 = 0X55, 0x55, ... 0x55, 0x54 (TODO: TBD)
    Sync Word(3) like  0x21, 0xA4, (0x23)
    Payload(5)   like  0x12, 0x34, 0x56, 0x78,  0x12
    CRC(2)       like  0x34, 0x56
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
  if (transmittedFlag) // if the last packet was sent, start a new transmission
  {
    radio.finishTransmit(); // Clear Transmitor
    transmittedFlag = false; // flag to indicate that a packet is being sent

    // =#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#

    byte buttonID = 0x04;
    Serial.print(F("[CC1101] Sending packet ... "));
    Serial.print(F("buttonID = 0x"));
    Serial.println(buttonID, HEX); 

    // A Minimal Packet
    // Preamble 0x54 (one bit error is allowed, like 0x14, 0xD4)
    // Sync Word 0x21 0xA4 0x23
    // SwitchID 0x01 0x02 0x03 0x04
    // ButtonID 0x04
    // CRC 0x00 0x00

    byte byteArr[] = {0x54,0x21, 0xA4,0x23, 0x01, 0x02, 0x03, 0x04, buttonID, 0x00, 0x00}; // 最后两个字节用于存放 CRC
    
    // place the CRC in the last two bytes
    byte payload[] = {byteArr[4], byteArr[5], byteArr[6], byteArr[7], byteArr[8]};
    crc_t crc = crc_init();
    crc = crc_update(crc, payload, 5);
    crc = crc_finalize(crc);
    byteArr[9] = (byte)(crc >> 8);
    byteArr[10] = (byte)(crc & 0xFF);

    // =#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#

    // send the packet
    int state = radio.startTransmit(byteArr, 11);

    if (state == RADIOLIB_ERR_NONE)
    {
      Serial.println(F("transmission finished!"));
    }
    else
    {
      Serial.print(F("failed, code "));
      Serial.println(state);
    }

    delay(3000);
  }
}

// #======================== Functions ========================#