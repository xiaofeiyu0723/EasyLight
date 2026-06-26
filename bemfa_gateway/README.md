# EasyLight Bemfa Gateway

This sketch bridges Bemfa/Mijia/Xiaoai switch commands to EasyLight RF packets.
It can route multiple Bemfa switch topics to multiple EasyLight controller IDs.

## Setup

1. In Bemfa, create MQTT switch topics.
   - The `006` suffix makes a topic a switch device.
   - Topic names may only contain letters and numbers.
   - For paired controllers, this sketch suggests topics like `el36f98d006`.
2. In Mijia, go to `My -> Other platform devices -> Add -> Bemfa`, sign in, then sync devices.
3. Edit `.config/bemfa_gateway_config.h`:
   - `WIFI_SSID`
   - `WIFI_PASSWORD`
   - `BEMFA_UID`
   - Optional bootstrap: `BEMFA_TOPIC` and `EASYLIGHT_CONTROLLER_ID`
   - Optional manual mappings: `STATIC_LIGHT_BINDINGS`
   - Optional pairing controls: `PAIR_BUTTON_PIN`, `PAIR_HOLD_MS`, `PAIR_WINDOW_MS`
4. Flash `bemfa_gateway.ino` to an ESP32 connected to CC1101.

## Manual Topic Mapping

If you manually created topics in Bemfa, keep those topic names and map them to controller IDs:

```cpp
#define STATIC_LIGHT_BINDINGS \
  {"lsecja5tY006", "36F98D"}, \
  {"AO0ZfVmPf006", "36AF6C"}
```

The first value is the Bemfa topic. The second value is the EasyLight controller ID captured from RF. Entries with an empty controller ID are ignored.

## Pairing Flow

1. Hold the pairing button for 3 seconds.
2. The gateway enters pairing mode for 60 seconds.
3. Trigger each light/controller so it emits a controller response packet.
4. The gateway extracts the controller ID, saves it, and sends the RF `ADD` gateway binding packet.
5. The gateway prints a suggested Bemfa topic, for example `el36f98d006`.
6. Create that MQTT topic in Bemfa and sync devices in Mijia.

The gateway stores bindings in ESP32 NVS. They survive reboot.

## Bemfa Topic Registration

Manual topic creation is enough for one device. For many switches, use Bemfa's batch API after real-name verification:

1. Get `secretID` and `secretKey` from the Bemfa console.
2. Put them in `.config/bemfa_gateway_config.h`.
3. Copy `devices.example.json` to `devices.json` and edit the topic/name/room list.
4. Validate first:

```bash
python create_bemfa_topics.py devices.json --dry-run
```

5. Create the MQTT switch topics:

```bash
python create_bemfa_topics.py devices.json
```

Bemfa topics must contain only letters and numbers. Switch topics should end with `006`. The batch API can create up to 99 topics per request.

## MQTT Behavior

The sketch subscribes to every saved Bemfa topic and handles:

- `on`: send RF ON packet
- `off`: send RF OFF packet

After sending RF, it publishes the expected state to `<topic>/up`.

## Notes

This first version reports the expected state after transmission. It captures controller IDs and validates controller response CRCs during pairing, but it does not yet use controller responses to confirm the final real light state.
