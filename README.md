# M365 BLE Dashboard

Wireless dashboard for Xiaomi M365/Pro scooter using ESP32 CYD 2.8" display.

![Dashboard](https://img.shields.io/badge/ESP32-CYD%202.8%22-blue)
![Status](https://img.shields.io/badge/status-working-green)

## Features

- Real-time speed, battery, voltage, current, power
- Odometer, trip distance, remaining range
- ESC and battery temperature monitoring
- Individual cell voltages with balance indicator
- Power and current graphs
- 3 screens: Main, Stats, Battery (tap to switch)

## Hardware

- **Board**: ESP32-2432S028 (CYD 2.8" with touch)
- **Display**: 320x240 ILI9341
- **Touch**: XPT2046

## Wiring

No external wiring needed - everything is on board.

## Building

### Option 1: PlatformIO (recommended)

1. Install [VS Code](https://code.visualstudio.com/) + [PlatformIO](https://platformio.org/)
2. Open project folder
3. Click Build, then Upload

### Option 2: Pre-built firmware

1. Download `firmware.bin` from releases
2. Flash using [ESP Web Flasher](https://esp.huhn.me/) or esptool:


## Usage

1. Power on ESP32 board
2. Turn on scooter
3. Dashboard auto-connects via Bluetooth
4. Tap screen to switch between pages

## Screens

**Main Screen**
- Speed (big number in center)
- Battery bar and percentage
- Voltage, current, power
- ESC and battery temperature
- ODO, trip, time
- Range, average speed, max speed

**Stats Screen**
- Power graph (last 60 seconds)
- Current graph (last 60 seconds)
- Real-time values

**Battery Screen**
- Battery percentage
- Voltage, current, power, capacity
- All 10 cell voltages
- Cell balance indicator

## Compatibility

Tested with:
- Xiaomi M365
- Xiaomi M365 Pro
- Custom firmware (ScooterHacking)

## Libraries

- TFT_eSPI
- NimBLE-Arduino
- XPT2046_Touchscreen

## Credits

Register map based on [ninebot-docs](https://github.com/etransport/ninebot-docs).
