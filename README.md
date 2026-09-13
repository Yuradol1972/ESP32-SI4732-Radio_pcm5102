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

## 🛠 Hardware Audio Routing & Modifications (The Mod)

### Why the Hardware Mod is Needed
In the stock pocket radio hardware (ATS-Mini / Banggood 1.9" IPS SI4732 receiver):
1. **SI4732 Direct Analog Audio**: The SI4732 tuner IC demodulates over-the-air radio signals (FM, AM, Shortwave, SSB) and sends analog audio lines (`LOUT`/`ROUT`) directly to the onboard audio power amplifier (typically an **NS4160** or **AD4150B** 8-pin class-D mono amplifier) driving the internal speaker and 3.5mm jack.
2. **No ESP32-S3 Internal DAC**: The ESP32-S3 microcontroller controls the SI4732 and display over digital buses (I2C/SPI), but **does not have internal hardware DACs** (unlike the older ESP32). Furthermore, there are no stock audio PCB traces between the ESP32-S3 and the amplifier.
3. **The Solution**: To stream digital internet radio through the speaker/headphone jack, we tap into three spare ESP32-S3 GPIO pins configured for digital I2S audio output.

---

### ESP32-S3 Audio Pinout:
| Signal | ESP32-S3 Pin | Physical Module Pin (WROOM-1) | Description |
|---|---|---|---|
| **BCLK** | `GPIO 11` | Pin 19 | I2S Bit Clock |
| **LRC (WS)** | `GPIO 12` | Pin 20 | I2S Word Select (Left/Right Clock) |
| **DOUT (DIN)** | `GPIO 13` | Pin 21 | I2S Serial Audio Data |
| **VIN** | `3.3V` or `VBAT` | 3.3V rail or Battery (+) | Power Supply (3.0V – 5.0V) |
| **GND** | `GND` | Common Ground | Ground Reference |

---

### Option 1: Micro I2S DAC Module (MAX98357A) — *Recommended for Hi-Fi Audio*

Using a miniature **MAX98357A** I2S 3W Mono Class-D amplifier breakout (approx. 15mm × 15mm) delivers crystal-clear digital sound, high volume, and low noise.

#### Bill of Materials:
- 1× **MAX98357A** I2S amplifier breakout module (or **PCM5102A** DAC for line-level headphone use)
- 30 AWG thin enameled copper or Kynar wrapping wire
- Kapton (polyimide) insulating tape

#### Wiring Diagram:
```
┌─────────────────────────────────────────────────────────────────┐
│                    ESP32-S3-WROOM-1 MODULE                      │
│                                                                 │
│   [GPIO 11] (BCLK)       [GPIO 12] (LRC)        [GPIO 13] (DOUT)│
└───────┬──────────────────────┬──────────────────────┬───────────┘
        │                      │                      │
        │                      │                      │
        ▼                      ▼                      ▼
┌─────────────────────────────────────────────────────────────────┐
│                 MAX98357A I2S AMPLIFIER BREAKOUT                │
│                                                                 │
│   BCLK                   LRC                    DIN             │
│   VIN (3.3V or VBAT)     GND                    GAIN (GND / NC) │
│                                                                 │
│   SPK+ (OUT+) ───────────────────────────────► [+] SPEAKER      │
│   SPK- (OUT-) ───────────────────────────────► [-] SPEAKER      │
└─────────────────────────────────────────────────────────────────┘
```

> [!TIP]
> **Speaker Connection**: Disconnect the speaker wires from the stock board amplifier output, or wire the MAX98357A outputs directly to the speaker terminals. If you wish to keep both amplifiers wired to the same speaker, place a miniature SPDT toggle switch or a summing resistor network (see below).

---

### Option 2: Software PDM Audio via RC Low-Pass Filter — *Minimal Components*

If you do not have an I2S DAC module on hand, the ESP32-S3 can generate high-frequency 1-bit Pulse-Density Modulation (PDM) audio on `GPIO 13`. A simple RC filter reconstructs this into an analog audio signal that injects directly into the onboard NS4160 amplifier.

#### Bill of Materials:
- 1× **1 kΩ** 1/8W resistor (or SMD 0805)
- 1× **10 nF** ceramic capacitor (high-frequency low-pass filter)
- 1× **10 µF** electrolytic or tantalum capacitor (DC blocking)

#### Schematic:
```
ESP32-S3 (GPIO 13) ───[ 1 kΩ Resistor ]───┬───[ + 10 µF Cap - ]───► Onboard NS4160 Input (Pin 4)
                                          │
                                       [10 nF]
                                          │
                                         GND
```

---

### Audio Mixing & Safe Isolation (SI4732 + ESP32)

To allow both the SI4732 tuner and the ESP32 Internet Radio to feed the same amplifier/speaker without electrical contention:

1. **Firmware Muting (Automatic)**:
   - When switched to the **`WEB`** band, the firmware commands the SI4732 chip to mute its analog output stage (`rx.setAudioMute(true)`).
   - When switched to any broadcast band (**FM, MW, SW, SSB**), the firmware shuts down the audio decoder, turns off the I2S clocks, and powers off the Wi-Fi modem (`esp_wifi_stop()`) to ensure zero RF hash.
2. **Hardware Summing Resistors**:
   - Insert a **1 kΩ series resistor** on the SI4732 audio line and a **1 kΩ series resistor** on the ESP32 audio line before tying them together at the amplifier input. This simple passive summing network prevents either chip's low-impedance output stage from back-driving the other.

---

### Step-by-Step Installation & Soldering Guide

1. **Disassembly**:
   - Remove the 4 screws on the back of the radio case.
   - Gently lift the back cover, taking care not to pull on the speaker wires or battery leads.
2. **Locate ESP32-S3 Pins**:
   - On the perimeter of the ESP32-S3 module, identify:
     - **GPIO 11** (BCLK)
     - **GPIO 12** (LRC/WS)
     - **GPIO 13** (DOUT/DIN)
     - **3.3V** and **GND** test pads or decoupling capacitors near the power switch.
3. **Prepare Wires**:
   - Cut five 4–5 cm lengths of thin 30 AWG wire.
   - Strip and tin the tips lightly.
4. **Solder Connections**:
   - Solder wires to `GPIO 11`, `GPIO 12`, `GPIO 13`, `3.3V`, and `GND`.
   - Connect the opposite ends to the MAX98357A module (or the RC filter circuit).
5. **Insulate & Mount**:
   - Wrap the MAX98357A board with a layer of **Kapton tape** to eliminate any risk of shorting against the battery or shield cans.
   - Secure the module in the empty space inside the case using double-sided foam tape.
6. **Testing**:
   - Power on the radio and verify FM/SW broadcast audio first.
   - Switch to **`WEB`** band, connect to Wi-Fi, and confirm internet radio streaming audio plays clearly through the speaker!
   - Close the case and replace the 4 screws.

---

## 🚀 Flashing the Firmware

### Option 1: Flash Pre-Compiled Binary (No Compilation Needed)
The repository includes ready-to-flash binaries in the [`binaries/`](binaries/) folder:
- **Merged Single-File Binary**: [`binaries/ats-mini.ino.merged.bin`](binaries/ats-mini.ino.merged.bin) (contains bootloader, partition table, and hybrid firmware in one 8MB image).

Flash using `esptool`:
```powershell
python -m esptool --chip esp32s3 -p <PORT> -b 921600 write_flash 0x0 binaries/ats-mini.ino.merged.bin
```

Or flash individual segments:
```powershell
python -m esptool --chip esp32s3 -p <PORT> -b 921600 write_flash 0x0 binaries/ats-mini.ino.bootloader.bin 0x8000 binaries/ats-mini.ino.partitions.bin 0x10000 binaries/ats-mini.ino.bin
```

### Option 2: Build & Flash with Automated Script
```powershell
cd ats-mini
.\flash.ps1
```
The script will automatically detect your ESP32-S3 COM port, invoke `arduino-cli`, and upload the firmware.

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

