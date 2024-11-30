/*
  A Kinetic energy switch Signal Receiver
  Use the CC1101 module
  Retrieve the 'switch_ID' and 'button_ID' from the signal

  Author:  Xiaofei YU, Lan HUANG 
  Date:    Nov 2024
*/

#include <Arduino.h>
#include <RadioLib.h>
#include <AceCRC.h>

/*
  CC1101 Connection
    SPI pins:
    - CSn  -> VSPI_CS    = GPIO5
    - SCK  -> VSPI_CLK   = GPIO18
    - GDO1 -> VSPI_MISO  = GPIO19
    - MOSI -> VSPI_MOSI  = GPIO23
    IO(interupt) pins:
    - GDO0 -> D1         = GPIO21
    - GDO2 -> D2         = GPIO22 (When using Transmit mode)

  Switch Packet Structure
    Preamble(X） like  01010101...0000100001 = 0X55, 0x55, ... 0x55, 0x54 (TODO: TBD)
    Sync Word(3) like  0x21, 0xA4, (0x23)
    Payload(5)   like  0x12, 0x34, 0x56, 0x78,  0x12
    CRC(2)       like  0x34, 0x56
*/

// #======================== Definitions ========================#

// Pinout
#define PIN_CS 5
#define PIN_GDO0 21
#define PIN_RST RADIOLIB_NC
#define PIN_GDO2 RADIOLIB_NC // Should be set to RADIOLIB_NC for RX only

// Serial
#define SERIAL_BAUD 9600

// Radio
#define RADIO_CARRIER_FREQUENCY 433.3   // MHz
#define RADIO_BIT_RATE 250.0            // kbps
#define RADIO_FREQUENCY_DEVIATION 125.0 // kHz
#define RADIO_RX_BANDWIDTH 270.0        // kHz
#define RADIO_OUTPUT_POWER 10           // dBm
#define RADIO_PREAMBLE_LENGTH 16        // bits

// Packet
#define PACKET_RSSI_THRESHOLD -100 // dBm (RSSI threshold for packet reception)
#define PACKET_LENGTH 20            // The Bytes you want to receive (after the sync word)
uint8_t SYNC_WORD[] = {0x21, 0xA4}; // The [21 A4] would be better, [A4 23] is not working, I don't know why
uint8_t SYNC_WORD_LENGTH = 2;
#define PACKET_THIRD_SYNC_WORD 0x23       // or Payload Prefix (TODO: Implement the third sync word)
using namespace ace_crc::crc16ccitt_byte; // Select the type of CRC algorithm we'll be using

// Special
volatile bool receivedFlag = false; // flag to indicate that a packet was received
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR // Place the function below in the RAM region
#endif
void setFlag(void) // void and no arguments
{
  receivedFlag = true; // we got a packet, set the flag
}

// #======================== Variables ========================#

CC1101 radio = new Module(PIN_CS, PIN_GDO0, PIN_RST, PIN_GDO2);

// #======================== Prototypes ========================#

void bytesToHexString(byte array[], unsigned int len, char buffer[]);

// #======================== Initialization ========================#

void setup()
{
  Serial.begin(SERIAL_BAUD);

  Serial.print(F("[CC1101] Initializing ... "));
  int state = radio.begin(RADIO_CARRIER_FREQUENCY, RADIO_BIT_RATE, RADIO_FREQUENCY_DEVIATION, RADIO_RX_BANDWIDTH, RADIO_OUTPUT_POWER, RADIO_PREAMBLE_LENGTH);
  radio.setCrcFiltering(false);
  radio.fixedPacketLengthMode(PACKET_LENGTH);
  radio.setSyncWord(SYNC_WORD, SYNC_WORD_LENGTH);
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

  radio.setPacketReceivedAction(setFlag); // Callback on packet received

  Serial.print(F("[CC1101] Starting to listen ... "));
  state = radio.startReceive();
  // Check Start
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
}

// # ======================== Main Loop ========================#

void loop()
{
  if (receivedFlag) //if a packet was received, process it
  {
    receivedFlag = false; // flag to indicate that a packet is being received

    /*
    String str;
    int state = radio.readData(str);
    */

    byte byteArr[PACKET_LENGTH]; // 0x23, [0x12, 0x34, 0x56, 0x78], [0x12], [0x34, 0x56]
    int state = radio.readData(byteArr, PACKET_LENGTH);

    
    // Print Content
    Serial.print(F("Received packet: "));
    for (uint8_t i = 0; i < PACKET_LENGTH; i++)
    {
      Serial.print(byteArr[i], HEX);
      Serial.print(F(" "));
    }
    Serial.println();
    

    if (state == RADIOLIB_ERR_NONE)
    {
      // if (radio.getRSSI() > PACKET_RSSI_THRESHOLD) {
      if (byteArr[0] == PACKET_THIRD_SYNC_WORD)
      {

        // Verify CRC and only continue if valid
        byte payload[] = {byteArr[1], byteArr[2], byteArr[3], byteArr[4], byteArr[5]}; // SwitchID + ButtonID

        crc_t crc = crc_init();
        crc = crc_update(crc, payload, 5);
        crc = crc_finalize(crc);
        unsigned long messageCRC = (byteArr[6] << 8) | byteArr[7];

        if ((unsigned long)crc == messageCRC)
        {
          char switchIDStr[9];
          char buttonIDStr[3];
          bytesToHexString(&byteArr[1], 4, switchIDStr);
          bytesToHexString(&byteArr[5], 1, buttonIDStr);

          Serial.print("SwitchID: ");
          Serial.print(switchIDStr);
          Serial.print("\tButtonID: ");
          Serial.print(buttonIDStr);
          Serial.print("\tRSSI: ");
          Serial.print(radio.getRSSI());
          Serial.print(" dBm\tLQI: ");
          Serial.println(radio.getLQI());
        } // CRC check
      } // Third Sync Word check
      //} // RSSI check
    }
    else
    {
      Serial.print(F("failed, code "));
      Serial.println(state);
    }

    radio.startReceive(); // put module back to listen mode
  }
}

// #======================== Functions ========================#

// Convert array of bytes into a string containing the HEX representation of the array
void bytesToHexString(byte array[], unsigned int len, char buffer[])
{
  for (unsigned int i = 0; i < len; i++)
  {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    buffer[i * 2 + 0] = nib1 < 0xA ? '0' + nib1 : 'A' + nib1 - 0xA;
    buffer[i * 2 + 1] = nib2 < 0xA ? '0' + nib2 : 'A' + nib2 - 0xA;
  }
  buffer[len * 2] = '\0';
}
