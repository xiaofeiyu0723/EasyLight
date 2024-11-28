#include <RadioLib.h>
#include <AceCRC.h>
// #define PACKET_LENGTH 8 // bytes

// Select the type of CRC algorithm we'll be using
using namespace ace_crc::crc16ccitt_byte;


void setFlag(void);
// CC1101 has the following connections:
// CS pin:    15
// GDO0 pin:  5
// RST pin:   unused
// GDO2 pin:  4
CC1101 radio = new Module(15, 5, RADIOLIB_NC, 4);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//CC1101 radio = RadioShield.ModuleA;

// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;

void setup() {
  Serial.begin(9600);

  // initialize CC1101 with default settings
  Serial.print(F("[CC1101] Initializing ... "));
  int state =  radio.begin(433.3, 250.0, 125.0, 270.0, 10, 32);
  radio.setCrcFiltering(false);
  // radio.fixedPacketLengthMode(PACKET_LENGTH);
  // radio.disableAddressFiltering();

  // CC1101不支持3bytes的sync word
  // uint8_t syncWord[] = {0x21, 0xA4};
  // radio.setSyncWord(syncWord, 2);
  uint8_t syncWord[] = {0x00};
  radio.setSyncWord(syncWord, 0);


  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // set the function that will be called
  // when packet transmission is finished
  radio.setPacketSentAction(setFlag);

}

// flag to indicate that a packet was sent
volatile bool transmittedFlag = true;

// this function is called when a complete packet
// is transmitted by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // we sent a packet, set the flag
  transmittedFlag = true;
}

// counter to keep track of transmitted packets
int count = 0;
byte buttonID=0x0F;

void loop() {
  // check if the previous transmission finished
  if(transmittedFlag) {
    // reset flag
    transmittedFlag = false;
    if (transmissionState == RADIOLIB_ERR_NONE) {
      // packet was successfully sent
      Serial.println(F("transmission finished!"));

      // NOTE: when using interrupt-driven transmit method,
      //       it is not possible to automatically measure
      //       transmission data rate using getDataRate()

    } else {
      Serial.print(F("failed, code "));
      Serial.println(transmissionState);
    }

    // clean up after transmission is finished
    // this will ensure transmitter is disabled,
    // RF switch is powered down etc.
    radio.finishTransmit();

    // wait a second before transmitting again
    delay(3000);

    // send another one
    Serial.print(F("[CC1101] Sending another packet ... "));
    Serial.print(F("buttonID = 0x"));
    Serial.println(buttonID, HEX);  // 以十六进制格式打印buttonID

    // you can also transmit byte array up to 256 bytes long
    // 添加
    // byte byteArr[] = {0xFF,0x55,0x55,0x55,0x55,0x54,0x21,0xA4,0x23,    0x03,0x0E,   0x3A,0x96,0x9D,    0x60,    0x8D,0x53,    0x9B,0x00,   0x36,0xAF,0x6C,   0x01}; 
    // 移除
    // byte byteArr[] = {0xFF,0x55,0x55,0x55,0x55,0x54,0x21,0xA4,0x23,    0x03,0x0E,   0x3A,0x96,0x9D,    0x60,    0xBD,0x30,    0x9B,0x00,   0x36,0xAF,0x6C,   0x02}; 
    // int state = radio.startTransmit(byteArr, 23);
    // 开
    // byte byteArr[] = {0xFF,0x55,0x55,0x55,0x55,0x54,0x21,0xA4,0x23,  0x03,0x10,   0x3A,0x96,0x9D,    0xE7,    0xBD,0x27,    0x9B,0x00,   0x36,0xAF,0x6C,   0x04,0x02,   0x01}; 
    // 关
    // byte byteArr[] = {0xFF,0x55,0x55,0x55,0x55,0x54,0x21,0xA4,0x23,  0x03,0x10,   0x3A,0x96,0x9D,    0xE7,    0xAD,0x06,    0x9B,0x00,   0x36,0xAF,0x6C,   0x04,0x02,   0x00}; 
    // int state = radio.startTransmit(byteArr, 25);

    // ping
    // byte byteArr[] = {0xFF,0x55,0x55,0x55,0x55,0x54,0x21,0xA4,0x23,    0x03,0x0F,   0x3A,0x96,0x9D,    0x2A,    0x07,0xA8,    0x9B,0x00,   0x36,0xAF,0x6C,   0x05,0x00}; 
    // int state = radio.startTransmit(byteArr, 24);
  }
}