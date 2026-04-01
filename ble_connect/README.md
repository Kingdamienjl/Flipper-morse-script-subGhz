# BLE Connect - Flipper Zero Native App

Native C application (.fap) for BLE device scanning, connection, and MAC address spoofing on the Flipper Zero.

**Status: Skeleton / Work in Progress** - Menu and GUI framework are functional. BLE operations are stubbed with TODO documentation for each feature.

## Planned Features

| Feature | Status | Description |
|---------|--------|-------------|
| Scan Devices | Stub | Discover nearby BLE devices (name, MAC, RSSI) |
| Connect | Stub | Initiate BLE connection to a target device |
| Spoof MAC | Stub | Change Flipper's BLE address to impersonate a device |
| Spam Pairing | Stub | Send repeated pairing requests to a target |

## Why Native C Instead of JavaScript?

The Flipper Zero JS engine only exposes `blebeacon` for one-way BLE advertising. To scan, connect, or interact with BLE devices, we need direct access to the STM32WB55 BLE HAL via native C code.

## Building

Requires [ufbt](https://github.com/flipperdevices/flipperzero-ufbt) (micro Flipper Build Tool):

```bash
# Install ufbt (one-time)
pip install ufbt

# Build the app
cd ble_connect/
ufbt

# Build and deploy to connected Flipper
ufbt launch
```

The compiled `.fap` will be in `dist/ble_connect.fap`. Copy it to your Flipper's SD card at `apps/Bluetooth/`.

## Technical Notes

- The Flipper Zero uses an **STM32WB55RG** with dual cores: Cortex-M4 (app) + Cortex-M0+ (BLE stack)
- BLE operations go through `furi_hal_bt` APIs which communicate with the M0+ core
- Scanning requires temporarily stopping the default BLE profile
- MAC spoofing alone does **not** bypass BLE bonding (bonded devices verify via IRK encryption keys)
- See `ble_connect.c` for detailed TODO comments on each feature's implementation approach

## References

- [Flipper Zero BLE HAL](https://developer.flipper.net/flipperzero/doxygen/furi__hal__bt_8h.html)
- [Wendigo - Flipper BLE Scanner](https://github.com/chris-bc/wendigo)
- [Flipper BLE MAC Spoofing](https://salmg.net/2023/02/03/flipper-zero-changing-bluetooth-mac-address/)
- [ufbt Documentation](https://github.com/flipperdevices/flipperzero-ufbt)
