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
HEX: 01 0C 00 36 F9 C6 90 B7 D8 01 00 41

Premable:       1010.... 01010100
Sync Word:      00100001 10100100 00100011
Device Type     00000001
Length:         00001100
TBD:            00000000
ControllerID_1: 00110110 11111001 = 0x36F9
TBD:            11000110 10010000 10110111 (CRC?)
ControllerID_2: 10001101 = 0xD8
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
  - controller_ID part 2 0xD8
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
  - controller_ID 0x36 F9 D8
  - command 0x01
    - on/off: 00000100 = 0x04
    - add: 00000001 = 0x01
    - remove: 00000010 = 0x02
  - parameter 0x00 (only on/off)
    - TBD: 00000010
  - status/value 0x00 (different when on/off)
    - on: 00000001 = 0x01
    - off: 00000000 = 0x00


# Others Test Notes

```
Try different error code rates in preamble and sync word
// 54 = 0101 0100 YES
// 14 = 0001 0100 YES
// 24 = 0011 0100 NO
// 04 = 0000 0100 NO
// C4 = 1100 0100 NO
// D4 = 1101 0100 NO

-------------------------------------------------
One Key Switch
// Press  00000100 = 4

Three Keys Switch
// Left   00000001 = 1
// Middle 00000100 = 4
// Right  00000010 = 2

Two Keys Switch
// Left   00000000 = 0
// Right  00000010 = 2

-------------------------------------------------

The Packet sent from the controller
when receiving a signal from the [kinetic switch]

SENT PACKET: 23 AA BB CC DD 04 00 00 (PRESS 7 bytes)
//              __ __ __ __ ++ [   ]
//           **                ** ** (CRC from 5 bytes)

RECEIVED PACKETs:
//23 01 0C 00 36 F9 C6 A3 86 D8 01 01 41 (RES_ON 12 bytes)
//23 01 0C 00 36 F9 C6 90 B7 D8 01 00 41 (RES_OFF 12 bytes)
//            __ __    [   ] __    ==
//**                ** ** **             (CRC from 9 bytes)

Other switch example
//23 01 0C 00 36 E6 27 90 69 08 01 00 41 
//            __ __    [   ] __    ==

//23 01 0C 00 37 40 C1 5A 66 71 01 00 81 (why 81?)
//23 01 0C 00 37 40 C1 5A 66 71 01 00 81
//23 01 0C 00 37 40 C1 69 57 71 01 01 81
//            __ __    [   ] __    ==

//23 01 0C 00 37 07 F9 31 43 8B 01 03 41 (error)
//23 01 0C 00 37 07 F9 02 70 8B 01 00 41
//            __ __    [   ] __    ==

Resp Structure:
//   CC LL LL AA AA ?? RR RR AA MM VV ??
- CC: Message Type?
- LL: Length (16 bits)
- AA: Address (24 bits)
- RR: CRC16-SPI-FUJITSU (16 bits)
- MM: 
- VV: Value (8 bits)

-------------------------------------------------

The Packet sent from the controller
when receiving a signal from the [gateway] (ON)

SENT PACKET: 23 03 10 3A 96 9D E7 00 00 9B 00 36 F9 D8 04 02 01 (ON 16 bytes)
//                    __ __ __                __ __ __ ++ ++ ++
//           **                ** ** **                         (CRC from 13 bytes)

RECEIVED PACKETs:
//23 04 0B 00 36 F9 B8 BD E5 D8 04 00 (RES 11 bytes)
//                     [   ]    ==
//**                ** ** **          (CRC from 8 bytes)

-------------------------------------------------

The Packet sent from the controller
when receiving a signal from the [gateway] (PING)

SENT PACKET: 23 03 0F 3A 96 9D 2A 00 00 9B 00 36 F9 D8 05 00 (PING 15 bytes)
//                    __ __ __                __ __ __ ++
//           **                ** ** **                      (CRC from 12 bytes)
//              CC LL GG GG GG ?? RR RR ?? ?? AA AA AA MM ??

RECEIVED PACKETs:
//23 04 0D 00 36 F9 91 82 32 D8 05 00 01 41 (RES_ON 13 bytes)
//23 04 0D 00 36 F9 91 B1 03 D8 05 00 00 41 (RES_OFF 13 bytes)
                       [   ]    ==    ==
//**                ** ** **                (CRC from 10 bytes)

Resp Structure:
//   CC LL LL AA AA ?? RR RR AA MM ?? VV ??
- CC: Message Type?
- LL: Length (16 bits)
- AA: Address (24 bits)
- RR: CRC16-SPI-FUJITSU (16 bits)
- MM: 
- VV: Value (8 bits)


-------------------------------------------------
```