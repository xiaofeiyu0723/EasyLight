# Binary sequence analysis of signals
Editor: Lan HUANG    Nov 2024

Collector: Xiaofei YU

## Signal types
### Kinetic Switch 🕹️
Main Components:
- switch_ID (24 bits)
- button_ID (8 bits)

Signal Sequence:
```
// A packet sent from a kinetic switch to the controller(light)
HEX: 54 21 A4 23 01 02 03 04 04 00 00

Premable:     1010.... 01010100
Sync Word:    00100001 10100100 00100011
SwitchID:     00000001 00000010 00000011 00000100
ButtonID:     00000100
CRC:          00000000 00000000
```
- Premable (32 bits?)
  - fixed 0x55, ..., 0x55, 0x54
- Sync Word (24 bits?)
  - fixed 0x21, 0x24, 0x23
- Data (40 bits)
  - switch_ID 0x01, 0x02, 0x03, 0x04
  - button_ID 0x05
    - One key switch:
      - Press: 0x04 = 00000100
      - Release: 0xC0 = 11000000
    - Two key switch:
      - Left Press: 0x00 = 00000000
      - Right Press: 0x02 = 00000010
      - Release: 0xC0 = 11000000
    - Three key switch:
      - Left Press: 0x01 = 00000001
      - Middle Press: 0x04 = 00000100
      - Right Press: 0x02 = 00000010
      - Release: 0xC0 = 11000000

- CRC (16 bits)
  - Algorithm: CRC-16/SPI-FUJITSU
  - Data Segment: switch_ID, button_ID (40 bits)




Minimum Effective Signal Length (When replaying): 
11 bytes
```
0x54,0x21,0xA4,0x23, 0x01,0x02,0x03,0x04, 0x04, 0x00, 0x00
```
### Controller (in light) 💡

Main Components:
- Device Type (8 bits)
- controller_ID (24 bits)

Signal Sequence:
```
// A packet sent from controller when receiving a signal from a kinetic switch(sometime)
HEX: 01 0C 00 36 F9 C6 90 B7 8D 01 00 41

Premable:       1010.... 01010100
Sync Word:      00100001 10100100 00100011
Device Type     00000001
Length:         00001100
TBD:            00000000
ControllerID_1: 00110110 11111001 = 0x36F9
TBD:            11000110 10010000 10110111 (CRC?)
ControllerID_2: 10001101 = 0x8D
TBD:            00000001 (parameter?)
Status:         00000000
TBD:            01000001 (tail?)
```

- Premable (32 bits?)
  - fixed 0x55, ..., 0x55, 0x54
- Sync Word (24 bits?)
  - fixed 0x21, 0x24, 0x23
- Data (12 bytes)
  - type 0x01
    - controller=0x01
    - gateway=0x03
  - TBD 0x0C (length?)
  - TBD 0X00
  - controller_ID part 1 0x36F9
  - TBD 0XC6 90 B7 (CRC?)
  - controller_ID part 2 0x8D
  - TBD 0x01 (parameter?)
  - status 0x00 (different when on/off)
  - TBD 0x41/0x81 (tail?)



### Gateway 🌐

Main Components:
- Device Type (8 bits)
- gateway_ID?
- operation type (8 bits)
- controller_ID (from controller signal 24 bits)
- command
- parameter
- status

Signal Sequence:
```
// A packet sent from gateway to controller to turn on the light
Premable:     1010.... 01010100
Sync Word:    00100001 10100100 00100011
Device Type   00000011
Length:       00010000
TBD:          00111010 100101101 0011101 (gateway_ID?)
Operation:    11100111
CRC:          10111101 00100111
TBD:          10011011 00000000
ControllerID: 00110110 10101111 01101100
Command:      00000100
Parameter:    00000010
Status:       00000001
```
- Premable (32 bits?)
  - fixed 0x55, ..., 0x55, 0x54
- Sync Word (24 bits?)
  - fixed 0x21, 0x24, 0x23
- Data (14/16 bytes)
  - type 0x03 
    - controller=0x01
    - gateway=0x03
  - length 0x00
  - TBD 0x00 0x00 0x00
  - operation type 0x01
    - on/off: 11100111 = 0xE7
    - add/delete: 01100000 = 0x60
  - CRC16-SPI-FUJITSU 0x00 0x00
  - TBD 0x0000 (gateway_ID?)
  - controller_ID 0x36 F9 8D
  - command 0x01
    - on/off: 00000100 = 0x04
    - add: 00000001 = 0x01
    - remove: 00000010 = 0x02
  - parameter 0x00 (only on/off)
    - TBD: 00000010
  - status/value 0x00 (different when on/off)
    - on: 00000001 = 0x01
    - off: 00000000 = 0x00
