# Flipper Zero Scripts Collection

A collection of scripts and apps for the Flipper Zero running Momentum firmware (post-Oct-2024 JS API).

## Scripts

| Script | Type | Description |
|--------|------|-------------|
| [morse_rf/](morse_rf/) | JS (mJS) | Morse Code RF Transmitter - generates Sub-GHz OOK signals encoding morse code messages |
| [ble_spam/](ble_spam/) | JS (mJS) | BLE Beacon Spam - broadcasts spoofed BLE advertisements to trigger pairing popups on Apple, Android, and Windows devices |
| [ble_connect/](ble_connect/) | Native C (.fap) | BLE Connect - skeleton app for BLE scanning, connection, MAC spoofing, and pairing (WIP, requires ufbt to compile) |

## Installation

### JavaScript Scripts (morse_rf, ble_spam)

1. Download the `.js` file from the script's directory
2. Copy it to your Flipper's SD card at `apps/Scripts/`
3. Run from: Momentum -> Apps -> Scripts

### Native Apps (ble_connect)

1. Install [ufbt](https://github.com/flipperdevices/flipperzero-ufbt): `pip install ufbt`
2. `cd ble_connect/ && ufbt`
3. Copy `dist/ble_connect.fap` to Flipper SD card at `apps/Bluetooth/`

See the [ble_connect README](ble_connect/README.md) for detailed build instructions.

## Firmware Compatibility

- **Momentum firmware** (recommended) - full JS module support including `blebeacon`
- Post-October 2024 JS API (`event_loop` + `gui/submenu` + `gui/text_input`)
- The native C app (ble_connect) may also work on other firmware builds when compiled with the appropriate SDK

## mJS Quirks Reference

Lessons learned from developing on the Flipper's mJS JavaScript engine:

- **No `+=` operator** - use `x = x + y` instead
- **No implicit type coercion** - cannot concatenate numbers with strings; use manual conversion
- **No closures** - pass outer references as extra arguments to `eventLoop.subscribe()`
- **No `to_lower_case()`, `parse_int()`, `to_string()`** - use manual implementations, `parseInt()`, and string concatenation
- **`submenuView.makeWith(props, children)`** - children (menu items) are the 2nd argument, not part of props
- **Storage API** - uses `storage.openFile()`, `file.read("ascii", n)`, `file.write()`, `file.close()`
- **`__dirpath` is not defined** - use hardcoded absolute paths (`/ext/...`)

## License

Apache License 2.0 - See [LICENSE](LICENSE) for details.
