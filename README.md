# 🌡️ Heater GSM Monitor

> Smart temperature monitoring with SMS alerts using ESP8266 and SIM800L

[![🚦 Build Verification](https://github.com/tubopo/heater-gsm-monitor/actions/workflows/verify.yml/badge.svg)](https://github.com/tubopo/heater-gsm-monitor/actions/workflows/verify.yml)
[![Platform](https://img.shields.io/badge/platform-ESP8266-blue.svg)](https://arduino-esp8266.readthedocs.io/)
[![Framework](https://img.shields.io/badge/framework-ESPHome-1f8b4c.svg)](https://esphome.io/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

## 🚀 Features

- **📱 SMS alerts** — warning, critical and sensor-disconnected notifications
- **🧮 Hard SMS budget** — 3 per hour and 10 per day by default, the daily count survives power loss; a held-back alert is sent later, not dropped
- **📞 Call for status** — the device hangs up and replies with the temperature and status
- **🌡️ DS18B20 monitoring** — 5-minute readings, glitch-tolerant, automatic recovery after a probe is re-plugged or replaced
- **📊 OLED display** — dark by default, wakes for 30 seconds on touch
- **📡 Modem bring-up** — baud scan (19200 first, then 9600/115200/57600/38400/4800) and a wait for network registration
- **🔋 Winter-safe** — headless production build with no WiFi/API/OTA
- **🛠️ Debug hotspot** — separate build with a WiFi access point, offline web UI, live logs, heap stats and OTA updates

## 📦 Hardware

| Component | Model | Purpose |
|-----------|-------|---------|
| 🖥️ **MCU** | LOLIN ESP8266 NodeMCU V3 | Main controller |
| 📡 **GSM** | SIM800L | SMS alerts |
| 🌡️ **Sensor** | DS18B20 | Temperature probe |
| 📺 **Display** | SSD1306 128x64 OLED | Status screen |
| 👆 **Button** | TTP223 Capacitive | Display toggle |
| ⚡ **Power** | LM2596 DC-DC + 2200μF cap | Regulated 4.2V for modem |

## 🔌 Wiring

```
┌─────────────────────────────────────────────────┐
│                LOLIN NodeMCU V3                 │
│                                                 │
│  D8 (GPIO15) ◄────────── SIM800L TX  (ESP RX)   │
│  D7 (GPIO13) ──────────► SIM800L RX  (ESP TX)   │
│  D0 (GPIO16) ──────────► SIM800L RST (optional) │
│                                                 │
│  D5 (GPIO14) ◄─────────► DS18B20 Data           │
│                          (+ 4.7kΩ to 3.3V)      │
│                                                 │
│  D2 (GPIO4)  ◄─────────► SSD1306 SDA            │
│  D1 (GPIO5)  ◄─────────► SSD1306 SCL            │
│                          (both + 4.7kΩ to 3.3V) │
│                                                 │
│  D6 (GPIO12) ◄────────── TTP223 OUT             │
│  D4 (GPIO2)  ──────────► Built-in LED           │
│                                                 │
│  GND ─────────────────── Common Ground          │
│  3.3V ────────────────── Logic Power            │
└─────────────────────────────────────────────────┘

⚠️ SIM800L power: 3.7-4.2V (use LM2596 + 2200μF cap)
⚠️ Pull-ups required: 4.7kΩ on DS18B20, SDA, SCL
```

`D8/GPIO15` is the ESP's **RX** and connects to the SIM800L **TX**. `D7/GPIO13`
is the ESP's **TX** and connects to the SIM800L **RX**.

## ⚠️ SIM PIN

**Disable the SIM PIN before use.** ESPHome's `sim800l` component cannot
unlock a PIN-protected SIM. Put the SIM in a phone, disable the PIN, then
install it in the modem.

## 🏗️ Repository Layout

```
esphome/
  heater-gsm-monitor.yaml        Production, headless
  heater-gsm-monitor.debug.yaml  AP hotspot + web UI + logs + OTA
  packages/common.yaml           Shared logic
  state_names.h                  State names shared by display, SMS and web UI
  fonts/                         Vendored Roboto Mono (SIL OFL)
  secrets.yaml.example           Secrets template
  requirements.txt               Pinned ESPHome version
site/                            Project page, published with GitHub Pages
Makefile                         Build, upload and log targets
```

## ✅ Getting Started

### 1. Install

```bash
python3 -m pip install -r esphome/requirements.txt
```

### 2. Configure secrets

```bash
cp esphome/secrets.yaml.example esphome/secrets.yaml
```

Edit `esphome/secrets.yaml`:

```yaml
phone_number: "+1234567890"      # recipient of alert/status SMS
ap_ssid: "heater-monitor-debug"  # debug hotspot name
ap_password: "heater-debug-1234" # debug hotspot password (8+ chars)
```

### 3. Build and upload

```bash
make esphome-config      # validate YAML
make esphome-build       # compile production firmware
make esphome-upload      # flash over USB
make esphome-logs        # USB logs at 115200
```

The production build is always flashed over USB; it intentionally has no WiFi
and no OTA.

### 4. Tune thresholds and SMS budget

All values live in `esphome/packages/common.yaml` under `substitutions`:

```yaml
alert_cooldown_s: "180"     # 3 minutes between automatic alerts
alert_hour_max: "3"         # hard cap per hour
alert_day_max: "10"         # hard cap per 24 h, stored in flash

temp_min: "10.0"            # normal range
temp_max: "45.0"
temp_critical_low: "5.0"    # critical range
temp_critical_high: "55.0"
temp_hysteresis: "1.0"      # margin needed to leave WARNING/CRITICAL
```

With the default 1 °C hysteresis, WARNING starts below 10 °C but only clears
at 11 °C (and likewise at the other thresholds), so a temperature hovering at
a threshold produces one alert instead of spending the whole SMS budget.

Automatic alerts are capped; **status replies to incoming calls are not
capped**, because you request them explicitly.

### 5. Display behavior

- The OLED is **off at boot** in the production build.
- Press the TTP223 button to turn it on for 30 seconds.
- Press again to turn it off immediately.
- While off, ESPHome stops the display poller, so there are no I2C writes and
  no display work at all.
- The built-in LED is on in NORMAL and off in WARNING/CRITICAL/DISCONNECTED.
- The `GSM:` line shows `NO REPLY` while the baud scan has not heard from the
  modem yet (check power and the two serial wires if it stays), `NO NETWORK`
  when the modem answers but is not registered (SIM, antenna, coverage), and
  `OK` once registered. After power-up the device waits up to 60 seconds for
  registration before normal operation starts.

### 6. SMS text

Every SMS, whether an automatic alert or a reply to a call, is the temperature
and the status:

```
21.5C NORMAL
```

If the probe cannot be read, the text is just `DISCONNECTED`.

To ask for the status, call the device's SIM number. It hangs up (`ATH`, then
`AT+CHUP` if the call is still ringing a second later), waits two seconds for
the call to clear, and sends the SMS.

### 7. Debug hotspot build

This is a **separate firmware**. Flash it when you need local diagnostics,
then reflash the production build.

```bash
make esphome-debug-build
make esphome-debug-upload
```

1. Connect to the debug WiFi network (`ap_ssid` / `ap_password` in
   `secrets.yaml`).
2. Open `http://192.168.4.1/`. The web UI is embedded in the debug firmware
   (`web_server: local: true`), so it works with no internet connection.
3. You get temperature, state, GSM RSSI, registration, the modem baud rate
   found by the scan, the touch button state, uptime, free heap, heap
   fragmentation, live logs, and a **Send Test SMS** button.
4. Logs can also be streamed with:

```bash
make esphome-debug-logs
# or: cd esphome && esphome logs heater-gsm-monitor.debug.yaml --device 192.168.4.1
```

5. Once a debug firmware is on the board, later debug builds can be flashed
   over the hotspot instead of USB. Build first, then join the hotspot:

```bash
make esphome-debug-build   # on your normal network
make esphome-debug-ota     # while connected to the debug hotspot
```

   The OTA password is `ap_password`. The production build has no WiFi and no
   OTA, so going back to it needs the USB cable.

The debug build enables WiFi and the web server, which costs extra RAM and
power. It is not intended for the winter deployment.

## 📊 Temperature States

| State | Range | LED | SMS |
|-------|-------|-----|-----|
| 🟢 **NORMAL** | 10°C – 45°C | ON | none |
| 🟡 **WARNING** | 5–10°C or 45–55°C | OFF | capped alert |
| 🔴 **CRITICAL** | ≤5°C or ≥55°C | OFF | capped alert |
| ⚫ **DISCONNECTED** | probe missing / unreadable | OFF | capped alert |

A single failed read is treated as a glitch: the probe is re-read every 30
seconds and only three failures in a row (about a minute) switch the state to
DISCONNECTED. The same 30-second retry rescans the 1-Wire bus, so a re-plugged
or replaced probe is picked up without a reboot.

## 🧮 SMS Budget

Automatic alerts:

- one alert per state change (to WARNING, CRITICAL or DISCONNECTED)
- at most **3 per hour**
- at most **10 per 24 hours**
- at least **3 minutes** apart
- the daily counter is written to flash before each SMS is sent, so a reboot
  or brownout loop cannot reset the budget; it clears after 24 hours of
  uninterrupted uptime

Status replies to incoming calls are not capped.

An alert that cannot go out yet — budget exhausted, cooldown running, or no
GSM registration — is not dropped. It stays pending and is retried every 30
seconds with the then-current state and temperature, and is cancelled if the
state returns to NORMAL first. The status-on-call reply always works, and the
display/LED still show the current state.

## 🧪 Verification

GitHub Actions runs `esphome config` and `esphome compile` for the production
and debug configs on every push.

## 📚 Resources

- [ESPHome](https://esphome.io/)
- [ESPHome SIM800L](https://esphome.io/components/sim800l/)
- [ESPHome SSD1306](https://esphome.io/components/display/ssd1306/)
- [ESP8266 Docs](https://arduino-esp8266.readthedocs.io/)
- [SIM800L Commands](http://m2msupport.net/m2msupport/sim800-series-at-command-manual/)
