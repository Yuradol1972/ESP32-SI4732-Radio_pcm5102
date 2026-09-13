# ESP32-S3 SI4732 Pocket Radio + Internet Radio (Hybrid Receiver)

This repository contains enhanced hybrid firmware for the **ESP32-S3 + SI4732** pocket all-band receiver (such as the Banggood 1.9" IPS mini radio). It seamlessly integrates the full-featured multi-band broadcast radio engine with an **Internet Radio streaming subsystem** ported from the high-stability ESP32-S3 audio engine.

---

## 🌟 Key Features

### 1. 28 Broadcast Bands Preserved (100% Stock Compatibility)
- **FM Band**: 64 – 108 MHz with RDS/RBDS station decoding, Program Service (PS), Radio Text (RT), and signal metrics.
- **Medium Wave (MW)**: MW1, MW2, MW3 bands covering European, American, and Asian grid channel spacing.
- **Shortwave (SW)**: 120M, 90M, 75M, 60M, 49M, 41M, 31M, 25M, 22M, 19M, 16M, 15M, 13M, 11M international broadcast bands.
- **Amateur Ham Radio**: 160M, 80M, 40M, 30M, 20M, 17M, 15M, 12M, 10M with genuine **LSB/USB Single Sideband** and fine BFO tuning.
- **Citizens Band (CB)**: 25.0 – 28.0 MHz AM.

### 2. Built-in Internet Radio Engine (`WEB` Band)
- **Zero RF Hash (Wi-Fi Isolation)**: When listening to FM/AM/SW/SSB, the Wi-Fi radio is **fully turned off** (`WiFi.disconnect(true); esp_wifi_stop()`) and CPU clock is lowered to 80 MHz, ensuring a pitch-black RF noise floor for weak shortwave signals.
- When switched to the **`WEB`** band, the SI4732 tuner is muted, CPU clocks up to 160 MHz, and the Wi-Fi modem engages to stream digital audio.
- **8MB Octal PSRAM TLS Allocator**: Custom `mbedtls` calloc/free allocator routed to PSRAM eliminates `-32512` memory exhaustion during HTTPS SSL handshakes.
- **HLS (.m3u8) & HTTP/HTTPS MP3/AAC Support**: Full redirect support (HTTP 302/301) and chunked live stream handling.

### 3. Dynamic LittleFS Favorites & Cloud Catalog Sync
- **Online Master Catalog**: The master station catalog is hosted right in this repository:
  [`stations.json`](https://raw.githubusercontent.com/sureshmagnolia/ESP32-SI4732-Radio/main/stations.json)
  Contains 270+ curated stations across Malayalam (Akashvani Thrissur relay, AIR Kochi, Manjeri, Ananthapuri), Hindi (Vividh Bharati, FM Gold, FM Rainbow, Live News 24x7), Tamil, Telugu, Kannada, Classical (Raagam), and international news (BBC World Service).
- **Web Portal Manager (`/webradio`)**:
  Visit `http://<radio-ip>/webradio` from any smartphone or PC browser:
  - Browse and search the 270+ station master catalog with instant search and language filters.
  - Click **`[ ⭐ Add ]`** to save any station into the ESP32-S3's LittleFS favorites.
  - Add your own custom stream URLs directly.
  - Reorder, play instantly on the device, or delete favorites.
- **Rotary Dial Tuning**: Only your saved favorites are loaded into the radio's active memory (`CH 01` to `CH N`), allowing you to rapidly flip through your favorite stations with the physical tuning knob!

---

## 🛠 Hardware Audio Routing (The Mod)

Because the stock receiver PCB routes analog audio directly from the SI4732 chip to the onboard speaker amplifier, the ESP32-S3 needs an audio path to deliver digital radio streams to the speaker/headphone jack.

### Spare ESP32-S3 GPIO Pins Used:
| Signal | ESP32-S3 Pin | Description |
|---|---|---|
| **BCLK** | `GPIO 11` | I2S Bit Clock |
| **LRC** | `GPIO 12` | I2S Word Select / Left-Right Clock |
| **DOUT** | `GPIO 13` | I2S Serial Data Out |

### Audio Connection Options:
1. **Option A (Micro I2S DAC - Recommended)**:
   Solder a miniature **MAX98357A** or **PCM5102A** DAC inside the radio shell, connected to GPIO 11, 12, 13, with its audio output wired to the speaker or amplifier input.
2. **Option B (Software PDM / RC Filter)**:
   Route the output pins through a simple RC filter (1 kΩ + 10 nF) and 10 µF coupling capacitors into the analog input pins of the onboard amplifier.

---

## 🚀 Flashing the Firmware

### Prerequisites
- Install [Arduino CLI](https://arduino.github.io/arduino-cli/latest/) or Arduino IDE with ESP32 board package (`esp32:esp32` version 3.x).
- Connect the ESP32-S3 receiver to your PC via USB-C.

### One-Click PowerShell Script
Run the automated flasher in PowerShell:
```powershell
cd ats-mini
.\flash.ps1
```
The script will automatically detect your ESP32-S3 COM port, invoke `arduino-cli`, and upload the bootloader, partition table, and hybrid firmware.

---

## 🌐 Configuring Wi-Fi & Web Portal

1. **First Boot**: In the radio menu or web config, configure your local Wi-Fi SSID and password (supports up to 3 saved networks).
2. **Connect**: Switch the band to **`WEB`** or enable Wi-Fi in the menu. The radio will connect to your router and display its IP address (e.g. `192.168.1.50` or `atsmini.local`).
3. **Open the Web Manager**:
   Open your browser to:
   ```
   http://<radio-ip>/webradio
   ```
4. **Select Stations**:
   Search the catalog, click **`[ ⭐ Add ]`**, and they will immediately sync to the radio's LittleFS memory and appear on your screen!

---

## 📡 Adding More Stations to the Online Catalog

To add or update stations in the master online catalog:
1. Edit [`stations.json`](stations.json) in this repository.
2. Commit and push your changes.
3. Every radio running this firmware will automatically see the new stations in its Web Portal immediately without needing any firmware re-flash!

