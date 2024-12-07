#include <Arduino.h>
#include <RadioLib.h>
#include <AceCRC.h>

// #======================== Definitions ========================#

using namespace ace_crc::crc16ccitt_byte; // Select CRC algorithm

// For ESP32 ONLY !!!
#define PIN_CS 5
#define PIN_GDO0 21
#define PIN_RST RADIOLIB_NC
#define PIN_GDO2 22

// RF Configurations
#define RADIO_CARRIER_FREQUENCY 433.3   // MHz
#define RADIO_BIT_RATE 250.0            // kbps
#define RADIO_FREQUENCY_DEVIATION 125.0 // kHz
#define RADIO_RX_BANDWIDTH 270.0        // kHz
#define RADIO_OUTPUT_POWER 10           // dBm
#define RADIO_PREAMBLE_LENGTH 32        // bits

// Packet
#define PACKET_RSSI_THRESHOLD -100  // dBm (RSSI threshold for packet reception)
#define PACKET_LENGTH 20            // The Bytes you want to receive (after the sync word)
uint8_t SYNC_WORD[] = {0x21, 0xA4}; // The [21 A4] would be better, [A4 23] is not working, I don't know why
uint8_t SYNC_WORD_LENGTH = 2;
#define PACKET_THIRD_SYNC_WORD 0x23

// #======================== Global Variables ========================#

CC1101 Radio = new Module(PIN_CS, PIN_GDO0, PIN_RST, PIN_GDO2);
Stream *Radio_Logger;
bool radio_initialized = false;

// Interrupts flags
volatile bool receivedFlag = false; // flag to indicate that a packet was received
volatile bool transmitFlag = true;  // flag to indicate that a packet was transmitted

// #======================== Prototypes ========================#

int radio_init();
int radio_handle();

bool radio_isInitialized();

int radio_setLoggerOutput(Stream *s);

// private
int radio_log_print(String message);

// interrupt
ICACHE_RAM_ATTR void setReceivedFlag(void);
ICACHE_RAM_ATTR void setTransmitFlag(void);

// #======================== Initialization ========================#

int radio_init()
{
    radio_log_print("[Radio] Initializing ...\n");
    int state = Radio.begin(RADIO_CARRIER_FREQUENCY, RADIO_BIT_RATE, RADIO_FREQUENCY_DEVIATION, RADIO_RX_BANDWIDTH, RADIO_OUTPUT_POWER, RADIO_PREAMBLE_LENGTH);
    Radio.setCrcFiltering(false);
    // Radio.fixedPacketLengthMode(PACKET_LENGTH);
    Radio.setSyncWord(SYNC_WORD, SYNC_WORD_LENGTH);

    if (state != RADIOLIB_ERR_NONE)
    {
        radio_log_print("[Radio] initialization failed: ");
        radio_log_print(String(state));
        return -1;
    }

    Radio.setPacketReceivedAction(setReceivedFlag); // Callback on packet received
    Radio.setPacketSentAction(setTransmitFlag);     // Callback on packet sent

    radio_log_print("[Radio] Initialized\n");
    radio_initialized = true;
    return 0;
}

// #======================== Main ========================#

int radio_handle()
{
    return 0;
}

// #======================== Functions ========================#

bool radio_isInitialized()
{
    return radio_initialized;
}

int radio_setLoggerOutput(Stream *s)
{
    Radio_Logger = s;
    return 0;
}

// private
int radio_log_print(String message)
{
    if (Radio_Logger)
    {
        Radio_Logger->print(message);
    }
    return 0;
}

// #======================== Callbacks ========================#

// #======================== Interrupt ========================#

ICACHE_RAM_ATTR void setReceivedFlag(void)
{
    receivedFlag = true;
}

ICACHE_RAM_ATTR void setTransmitFlag(void)
{
    transmitFlag = true;
}

// #======================== End ========================#
