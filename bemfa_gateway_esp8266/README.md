# EasyLight Bemfa Gateway for ESP8266 D1 mini

This sketch is the ESP8266/D1 mini version of the Bemfa bridge.

It connects to Bemfa MQTT, subscribes to the configured switch topics, serves a small pairing web page from a setup hotspot, and sends EasyLight 433 MHz RF packets when a topic has a mapped controller ID.

## D1 Mini Wiring

| CC1101 | D1 mini | ESP8266 GPIO |
|---|---|---|
| VCC | 3V3 | 3.3V |
| GND | G | GND |
| SCK | D5 | GPIO14 |
| MISO / SO / GDO1 | D6 | GPIO12 |
| MOSI / SI | D7 | GPIO13 |
| CSN / CS | D8 | GPIO15 |
| GDO0 | D1 | GPIO5 |
| GDO2 | D2 | GPIO4 |
| RST | leave unconnected | `RADIOLIB_NC` |

Use 3.3V only. Do not power CC1101 from 5V.

## Arduino IDE

1. Install ESP8266 board support.
2. Select board: `LOLIN(WEMOS) D1 R2 & mini` or a compatible D1 mini board.
3. Select port: `COM3`.
4. Install libraries:
   - `PubSubClient`
   - `RadioLib`
   - `AceCRC`
   - `ArduinoJson`
5. Copy `bemfa_gateway_esp8266_config.example.h` to `bemfa_gateway_esp8266_config.h`.
6. Fill WiFi and Bemfa UID. Topic mappings are optional fallback entries; the gateway can sync switch topics from Bemfa after WiFi connects.
7. Upload `bemfa_gateway_esp8266.ino`.
8. Open Serial Monitor at `115200`.

## Web Pairing

The D1 mini has no BOOT button, so pairing is done from a phone browser.

1. Power the D1 mini.
2. It creates a WiFi hotspot named like `EasyLight-ABC123`.
3. Connect your phone to that hotspot.
   - Password: `easylight`
4. Open:

```text
http://192.168.4.1
```

5. Set your home WiFi SSID and password from the `WiFi` form. After it connects to your router, the page shows its LAN URL, for example `http://192.168.66.155`.
6. The gateway syncs switch topics from Bemfa after WiFi connects. You can also tap `Sync Bemfa` manually.
7. Choose the Bemfa topic you want to bind and tap `Pair`.
8. Within 60 seconds, trigger the corresponding light/controller so it emits an RF response.
9. The gateway extracts the controller ID, sends the RF gateway `ADD` packet, and saves the topic mapping in ESP8266 flash.

Use `Unpair` on the same page to send the RF gateway `DELETE` packet and clear the saved mapping.

## RF Gateway Protocol

This sketch uses only the EasyLight gateway protocol, not the direct kinetic-switch replay protocol.

- Pair/add gateway:
  `54 21 A4 23 03 0E 3A 96 9D 60 <CRC> 9B 00 <controllerId[3]> 01`
- Unpair/delete gateway:
  `54 21 A4 23 03 0E 3A 96 9D 60 <CRC> 9B 00 <controllerId[3]> 02`
- Power on/off:
  `54 21 A4 23 03 10 3A 96 9D E7 <CRC> 9B 00 <controllerId[3]> 04 02 <01|00>`

The controller ID is read from valid controller response packets as bytes 4, 5, and 9 after the leading `23` byte.

## Status Sync

The pairing page shows live local status:

- WiFi connection
- MQTT connection
- CC1101 radio status
- Status topic

When MQTT is connected, the gateway publishes status every 30 seconds to the first configured topic's status channel, for example:

```text
lsecja5tY006/status/up
```

Payload example:

```text
wifi:connected;mqtt:connected;radio:ready;ip:192.168.1.23;ap:192.168.4.1
```

## Power Control

For `on`/`off` commands, the MQTT callback only queues the RF work. The main loop sends queued RF packets with at least 750 ms between transmissions, then tracks each command's `POWER` ACK in the background:

```text
54 21 A4 23 03 10 3A 96 9D E7 <CRC> 9B 00 <controllerId[3]> 04 02 <01|00>
```

If no `POWER` ACK arrives within 2 seconds, the gateway queues one retry for 2 seconds later. Fresh user commands are always sent before retries. A new command for the same topic replaces any queued older command and cancels that receiver's previous pending ACK, so rapid toggles do not get pulled backward by stale retries. A `REJECT` ACK stops the retry. The EasyLight gateway `PING` command can read state on some receivers, but it is not used automatically because some receivers visibly blink when pinged.

If CC1101 fails to initialize, `radio` will show an error code such as `error:-2`.

## Notes

- D8/GPIO15 is a boot strap pin. If upload or boot fails, temporarily disconnect CC1101 CSN from D8 while flashing.
- Entries with an empty controller ID still subscribe to Bemfa and log commands, so you can test Mijia/Bemfa before the RF side is ready.
- You may also pre-fill a controller ID in the config instead of using web pairing, for example:

```cpp
{"88msRmqGQ006", "36F98D", "Bedroom Light"}
```
