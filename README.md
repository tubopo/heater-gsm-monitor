# 🌡️ Heater GSM Monitor

> Smart temperature monitoring with SMS alerts using ESP8266 and SIM800L

[![Build Status](https://github.com/tubopo/heater-gsm-monitor/workflows/Build%20Verification/badge.svg)](https://github.com/tubopo/heater-gsm-monitor/actions)
[![Platform](https://img.shields.io/badge/platform-ESP8266-blue.svg)](https://arduino-esp8266.readthedocs.io/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

## 🚀 Features

- **📱 SMS Alerts** - Automatic notifications when temperature goes out of range
- **🌡️ Real-time Monitoring** - DS18B20 sensor reads every 10 seconds
- **📊 OLED Display** - Shows temperature, GSM signal, and status
- **🔘 Touch Control** - Capacitive button to toggle display
- **🔧 Modular Design** - Clean header-only architecture

## 📦 Hardware

| Component | Model | Purpose |
|-----------|-------|---------|
| 🖥️ **MCU** | LOLIN ESP8266 NodeMCU V3 | Main controller |
| 📡 **GSM** | SIM800L | SMS alerts |
| 🌡️ **Sensor** | DS18B20 | Temperature probe |
| 📺 **Display** | SSD1306 128x64 OLED | Status screen |
| 👆 **Button** | TTP223 Capacitive | Display toggle |
| ⚡ **Power** | LM2596 DC-DC + 2200μF cap | Regulated 4.2V for modem |

## 🔌 Wiring Diagram

### ESP8266 Pin Connections

```
┌─────────────────────────────────────────────────┐
│                LOLIN NodeMCU V3                 │
│                                                 │
│  D8 (GPIO15) ──────────► SIM800L TX             │
│  D7 (GPIO13) ◄────────── SIM800L RX             │
│  D0 (GPIO16) ──────────► SIM800L RST (optional) │
│                                                 │
│  D5 (GPIO14) ──────────► DS18B20 Data           │
│                          (+ 4.7kΩ to 3.3V)      │
│                                                 │
│  D2 (GPIO4)  ──────────► SSD1306 SDA            │
│  D1 (GPIO5)  ──────────► SSD1306 SCL            │
│                          (both + 4.7kΩ to 3.3V) │
│                                                 │
│  D6 (GPIO12) ◄────────── TTP223 OUT             │
│  D4 (GPIO2)  ──────────► Built-in LED           │
│                                                 │
│  GND ─────────────────── Common Ground          │
│  3.3V ────────────────── Logic Power            │
└─────────────────────────────────────────────────┘

⚠️ SIM800L Power: 3.7-4.2V (use LM2596 + 2200μF cap)
⚠️ Pull-ups Required: 4.7kΩ on DS18B20, SDA, SCL
```

## ⚡ Quick Start

### 1️⃣ Install Dependencies

```bash
# Using Makefile (recommended)
make dependencies

# Or manually with Arduino CLI
arduino-cli core install esp8266:esp8266
arduino-cli lib install "TinyGSM" "DallasTemperature" "OneWire" \
                         "Adafruit SSD1306" "Adafruit GFX Library"
```

### 2️⃣ Configure Secrets

Create `heater-gsm-monitor/arduino_secrets.h`:

```cpp
#define SECRET_PHONE_NUMBER "+1234567890"  // Your phone
#define SECRET_SIM_PIN "1234"              // SIM PIN or ""
```

### 3️⃣ Build & Upload

```bash
# Build
make build

# Upload (adjust port)
make upload ARDUINO_PORT=/dev/cu.usbserial-110

# Monitor serial (9600 baud)
make monitor
```

### 4️⃣ Adjust Thresholds

Edit `config.h`:

```cpp
#define TEMP_MIN 5.0              // Normal range min (°C)
#define TEMP_MAX 45.0             // Normal range max (°C)
#define TEMP_CRITICAL_LOW -10.0   // Critical low threshold
#define TEMP_CRITICAL_HIGH 50.0   // Critical high threshold
#define ALERT_COOLDOWN 180000     // 3 min between SMS (ms)
```

## 📊 Operation

### Temperature States

| State | Range | LED | Action |
|-------|-------|-----|--------|
| 🟢 **NORMAL** | 5°C - 45°C | ON | No alert |
| 🟡 **WARNING** | Outside normal | OFF | SMS sent |
| 🔴 **CRITICAL** | ≤-10°C or ≥50°C | Blink | SMS sent |

*Alert cooldown: 3 minutes (prevents spam)*

## 📚 Resources

- [ESP8266 Docs](https://arduino-esp8266.readthedocs.io/)
- [TinyGSM Library](https://github.com/vshymanskyy/TinyGSM)
- [SIM800L Commands](http://m2msupport.net/m2msupport/sim800-series-at-command-manual/)
