# NeoKey WLED Commander

A physical 4-key controller for [WLED](https://kno.wled.ge/) lighting presets, built on the **Seeed XIAO ESP32-S3** and the **Adafruit NeoKey 1x4** mechanical keypad.

Press a key → your WLED lights instantly switch to the assigned preset. Each key glows the color of the preset it controls.

![Platform](https://img.shields.io/badge/platform-ESP32--S3-blue)
![Framework](https://img.shields.io/badge/framework-Arduino-teal)
![License](https://img.shields.io/badge/license-MIT-green)

---

## Features

- **One-touch preset switching** — Each of the 4 mechanical keys activates a WLED preset via HTTP API.
- **Live color sync** — Keys glow the actual color of their assigned WLED preset, fetched at boot and polled in the background.
- **Active preset highlight** — The currently active preset key glows at full brightness; others dim to ~25%.
- **Boot-up KITT animation** — A red scanner sweep plays while the device connects to Wi-Fi.
- **Captive portal setup** — On first boot (or factory reset), an on-device Wi-Fi hotspot walks you through configuration with a mobile-friendly web UI — no coding required.
- **Background web dashboard** — After setup, visit `http://wled-commander.local` to reconfigure WLED targets, adjust brightness, and change animation styles without rebooting.
- **Visual Keymapper** — Map each key to a specific WLED preset. Use the **Auto-Map** feature to dynamically pull the lowest 4 presets, and assign **Custom Labels** that appear right on the dashboard keys!
- **Over-the-Air updates** — The device checks GitHub daily for new firmware. A cyan pulse on Key 0 means an update is available. Long-press to install, short-press to dismiss for 48 hours.
- **Factory reset** — Hold Key 0 + Key 3 at boot for 3 seconds to wipe all settings and re-enter the setup portal.
- **Serial diagnostics** — Full serial console with `status`, `help`, `scan`, `heap`, and `key N` commands for debugging.

> 📖 **Need help setting up?** Read the comprehensive **[User Guide](USER_GUIDE.md)**!

---

## Hardware

| Component | Description |
|---|---|
| **MCU** | [Seeed Studio XIAO ESP32-S3](https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html) |
| **Keypad** | [Adafruit NeoKey 1x4 QT (I2C)](https://www.adafruit.com/product/4980) |
| **Connection** | I2C via STEMMA QT / Qwiic cable |
| **Switches** | Cherry MX compatible (your choice of switch) |

### Wiring

| Signal | XIAO Pin | NeoKey Pin |
|---|---|---|
| SDA | D1 (GPIO 2) | SDA |
| SCL | D2 (GPIO 3) | SCL |
| 3V3 | 3V3 | VIN |
| GND | GND | GND |

---

## Quick Start

### 1. Flash the Firmware

**Option A — PlatformIO (recommended):**
```bash
# Clone and build
git clone https://github.com/thefoeyouknow/NeoKey-WLED-Commander.git
cd NeoKey-WLED-Commander
pio run -t upload
```

**Option B — Pre-built binary:**
Download the latest `firmware_v*.bin` from the [Releases](https://github.com/thefoeyouknow/NeoKey-WLED-Commander/releases) page and flash with `esptool.py` or the ESP Web Flasher.

### 2. First-Time Setup

1. Power on the device. It will create a Wi-Fi hotspot called **WLED-Key-Setup**.
2. Connect to it from your phone or laptop.
3. A setup page will open automatically. Enter your Wi-Fi credentials and WLED device address.
4. The device reboots, connects to your network, and fetches your WLED presets. Done!

### 3. Assign Presets

The setup portal lets you map each of the 4 keys to any WLED preset ID. The device will fetch the color and name of each preset automatically. Check out the **[User Guide](USER_GUIDE.md)** for detailed mapping instructions.

---

## Web Dashboard

Once running, visit **`http://wled-commander.local`** from any device on the same network to:

- Change WLED target addresses
- Adjust active/inactive LED brightness
- Choose key-press animation style (flash, pulse, or rainbow)
- All changes apply live — no reboot needed

---

## OTA Updates

The device checks for firmware updates from this repository once every 24 hours.

- **Cyan double-flash on Key 0** = An update is available.
- **Short press Key 0** = Dismiss for 48 hours.
- **Long press Key 0 (1.5s)** = Download and install the update. The device reboots automatically.

---

## Serial Commands

Connect via USB at 115200 baud:

| Command | Description |
|---|---|
| `help` | Show available commands |
| `status` | Device state, Wi-Fi, presets, memory |
| `version` | Firmware version |
| `key N` | Simulate pressing key 0–3 |
| `scan` | I2C bus scan |
| `heap` | Free memory report |
| `reset` | Factory reset (wipe NVS + reboot) |
| `reboot` | Restart device |
| `portal` | Enter captive portal setup mode |

---

## Project Structure

```
NeoKey-WLED-Commander/
├── src/
│   ├── main.cpp              # Main orchestrator and loop
│   ├── config.h              # All compile-time constants
│   ├── neokey_driver.h/cpp   # I2C keypad + NeoPixel driver
│   ├── led_animator.h/cpp    # Animation state machine
│   ├── wifi_manager.h/cpp    # Wi-Fi STA + mDNS resolution
│   ├── wled_client.h/cpp     # WLED JSON API client
│   ├── nvs_store.h/cpp       # Non-volatile config storage
│   ├── captive_portal.h/cpp  # Setup & background web server
│   ├── portal_html.h         # Embedded HTML/CSS/JS
│   └── ota_manager.h/cpp     # GitHub OTA update engine
├── version.json              # Current release info (polled by devices)
├── platformio.ini            # Build configuration
└── README.md
```

---

## Building from Source

Requires [PlatformIO](https://platformio.org/).

```bash
# Build
pio run

# Upload to connected device
pio run -t upload

# Monitor serial output
pio device monitor -b 115200
```

---

## License

MIT — do whatever you want with it.
