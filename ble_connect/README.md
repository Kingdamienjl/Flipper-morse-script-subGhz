# BLE Connect v0.2 - Flipper Zero Native App

Full-featured BLE toolkit for the Flipper Zero — beacon spam, MAC address spoofing, and configurable beacon settings with a polished multi-view GUI.

## Features

| Feature | Status | Description |
|---------|--------|-------------|
| BLE Spam Attack | **Working** | Broadcast spoofed BLE ads (Apple, Android, Windows) with randomized MACs |
| MAC Address Spoof | **Working** | Set custom MAC address for beacon advertising |
| Beacon Settings | **Working** | Configure TX power (-40dBm to 0dBm) and interval (20ms-1000ms) |
| Scan Devices | Planned | BLE device discovery (requires HCI central mode) |
| Connect to Device | Planned | BLE connection initiation (requires central mode) |

## Spam Attack Modes

| Mode | Target | Protocol | Effect |
|------|--------|----------|--------|
| Apple | iOS/macOS | Continuity (Proximity Pairing) | AirPods/AppleTV/Beats pairing popups |
| Android | Android | Google Fast Pair | "Device found nearby" notifications |
| Windows | Windows 10/11 | Microsoft Swift Pair | "New Bluetooth device" popups |
| All | Everything | Cycles all protocols | Rotates through all device types |

### Spoofed Devices
- **Apple (10 types)**: AirPods, AirPods Pro, AirPods Max, AirPods Gen3, Beats Fit Pro, Beats Solo Pro, AppleTV Setup, AppleTV Keyboard, New Device, Transfer Number
- **Android (8 types)**: Pixel Buds, Pixel Buds Pro, Galaxy Buds2, Galaxy Buds Live, Galaxy Buds Pro, Sony WF-1000XM4, JBL Tune Flex, Nothing Ear 1
- **Windows**: Generic Swift Pair device

## GUI Structure

```
Main Menu
├── BLE Spam Attack
│   ├── Apple (iOS popups)
│   ├── Android (Fast Pair)
│   ├── Windows (Swift Pair)
│   ├── All Devices
│   └── Stop Spam
├── MAC Address Spoof (hex input: AABBCCDDEEFF)
├── Scan Devices (info screen - planned feature)
├── Beacon Settings
│   ├── TX Power: -40dBm to 0dBm
│   └── Interval: 20ms to 1000ms
└── About
```

The spam status screen shows real-time packet count, current mode, and which device is being spoofed. Press BACK to stop.

## Building

Requires [ufbt](https://github.com/flipperdevices/flipperzero-ufbt):

```bash
pip install ufbt

cd ble_connect/
ufbt          # Build .fap
ufbt launch   # Build + deploy to connected Flipper
```

Output: `dist/ble_connect.fap` — copy to Flipper SD card at `apps/Bluetooth/`

## Technical Details

- Uses `furi_hal_bt_extra_beacon_*` API for BLE advertising (does not interfere with Flipper's main BLE connection)
- `FuriTimer` for periodic beacon cycling during spam
- `furi_hal_random_fill_buf()` for cryptographic-quality random MACs
- ViewDispatcher with 10 views: 2x Submenu, 4x Widget, VariableItemList, TextInput, Loading, Popup
- Proper memory management — all views allocated/freed cleanly
- 16KB stack for BLE operations

## Limitations

- **BLE Scanning** requires HCI central mode which is not publicly exposed in the Flipper BLE HAL. The STM32WB55 hardware supports it, but the firmware's BLE stack only operates in peripheral mode. See [Wendigo](https://github.com/chris-bc/wendigo) for experimental scanning.
- **BLE Connections** similarly require central mode. Cannot initiate connections to other devices.
- **MAC Spoofing** changes the beacon advertisement address only. It does NOT bypass BLE bonding (paired devices verify via encrypted IRK keys).
- **iOS 17.2+** throttles BLE spam notifications. Older iOS versions are more affected.

## References

- [Flipper BLE HAL](https://developer.flipper.net/flipperzero/doxygen/furi__hal__bt_8h.html)
- [Extra Beacon API](https://developer.flipper.net/flipperzero/doxygen/extra__beacon_8h.html)
- [Wendigo - BLE Scanner](https://github.com/chris-bc/wendigo)
- [Apple BLE Spam](https://github.com/noproto/apple_ble_spam_ofw)
- [ufbt Documentation](https://github.com/flipperdevices/flipperzero-ufbt)
