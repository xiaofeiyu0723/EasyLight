#include <Arduino.h>
#include <RadioLib.h>
#include <AceCRC.h>

// Select the type of CRC algorithm we'll be using
using namespace ace_crc::crc16ccitt_byte;

#define PACKET_LENGTH 8 // bytes
#define MIN_RSSI -100 // dBm

void bytesToHexString(byte array[], unsigned int len, char buffer[]);

// CC1101 has the following connections:
// CS pin:    15
// GDO0 pin:  5
// RST pin:   unused
// GDO2 pin:  unused
// CC1101 radio = new Module(10, 2, RADIOLIB_NC, RADIOLIB_NC);
CC1101 radio = new Module(15, 5, RADIOLIB_NC, RADIOLIB_NC);

void setup() {
  Serial.begin(9600);

  // initialize CC1101 with default settings
  Serial.print(F("[CC1101] Initializing ... "));
  // int state = radio.begin();

  int state = radio.begin(433.3, 250.0, 125.0, 270.0, 10, 32);

  radio.setCrcFiltering(false);
  radio.fixedPacketLengthMode(PACKET_LENGTH);
  // CC1101不支持3bytes的sync word 0x
  uint8_t syncWord[] = { 0x21, 0xA4};
  radio.setSyncWord(syncWord, 2);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // set the function that will be called
  // when new packet is received
  radio.setPacketReceivedAction(setFlag);

  // start listening for packets
  Serial.print(F("[CC1101] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // if needed, 'listen' mode can be disabled by calling
  // any of the following methods:
  //
  // radio.standby()
  // radio.sleep()
  // radio.transmit();
  // radio.receive();
  // radio.readData();
}

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // we got a packet, set the flag
  receivedFlag = true;
}

void loop() {
  // check if the flag is set
  if (receivedFlag) {
    // reset flag
    receivedFlag = false;

    // you can read received data as an Arduino String
    // String str;
    // int state = radio.readData(str);

    // you can also read received data as byte array
    byte byteArr[PACKET_LENGTH];
    int state = radio.readData(byteArr, PACKET_LENGTH);

    if (state == RADIOLIB_ERR_NONE) {
      // if (radio.getRSSI() > MIN_RSSI) {
      // Verify CRC and only continue if valid
      byte messageArr[] = {byteArr[1], byteArr[2], byteArr[3],byteArr[4],byteArr[5]};
      unsigned long messageCRC = (byteArr[6] << 8) | byteArr[7];
      crc_t crc = crc_init();
      crc = crc_update(crc, messageArr, 5);
      crc = crc_finalize(crc);
      if (((unsigned long) crc) == messageCRC) {
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
      }
      // }
    } else{
      // some other error occurred
      Serial.print(F("failed, code "));
      Serial.println(state);
    }

    // put module back to listen mode
    radio.startReceive();
  }
}

// Convert array of bytes into a string containing the HEX representation of the array
void bytesToHexString(byte array[], unsigned int len, char buffer[]) {
    for (unsigned int i = 0; i < len; i++) {
        byte nib1 = (array[i] >> 4) & 0x0F;
        byte nib2 = (array[i] >> 0) & 0x0F;
        buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
        buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
    }
    buffer[len*2] = '\0';
}
