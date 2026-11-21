# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

ESPHome firmware for an ESP8266 (NodeMCU) heater monitor: a DS18B20 probe, a SIM800L GSM modem, an SSD1306 OLED and a TTP223 touch button. It sends an SMS when the temperature leaves its allowed range and replies with an SMS when called. `README.md` has the wiring, thresholds and user-facing behavior.

## Commands

ESPHome is pinned in `esphome/requirements.txt`. All targets run from the repo root.

```bash
make esphome-config          # validate the production config
make esphome-build           # compile production firmware
make esphome-upload          # flash production over USB (ESPHOME_PORT=/dev/cu...)
make esphome-logs            # production logs over USB

make esphome-debug-build     # compile the debug firmware
make esphome-debug-upload    # flash debug over USB
make esphome-debug-ota       # flash debug over its hotspot (192.168.4.1)
make esphome-debug-logs      # debug logs over its hotspot
```

There are no unit tests or linters. Verification is `esphome config` plus `esphome compile` for **both** entry points, which is also what CI (`.github/workflows/verify.yml`) runs. A change is not verified on hardware unless someone flashes it; say so.

CI copies `esphome/secrets.yaml.example` to `secrets.yaml`, so every new `!secret` key must be added to the example file or CI fails.

## Architecture

Two entry points share one package:

- `esphome/heater-gsm-monitor.yaml` — production. No WiFi, no API, no OTA; flashed over USB only.
- `esphome/heater-gsm-monitor.debug.yaml` — adds a WiFi access point, embedded web UI, OTA and diagnostic entities. Never joins a router.
- `esphome/packages/common.yaml` — all logic. The entry points only set `device_name` and `display_boot_on`.

Logic in `common.yaml`, as lambdas on ESPHome triggers:

- **State** is the `state` global: an int index into `STATE_NAMES` in `esphome/state_names.h` (0 NORMAL, 1 WARNING, 2 CRITICAL, 3 DISCONNECTED). It is only changed in the temperature sensor's `on_value`, which applies hysteresis and requires three failed reads in a row before DISCONNECTED.
- **Alerts**: a state change sets `alert_pending`; the `send_alert` script sends when the budget (cooldown, hourly and daily caps) allows and GSM is registered. It runs on every reading and from the 30 s interval, so a blocked alert is deferred, not dropped.
- **30 s interval**: when there is no valid reading it rescans the 1-Wire bus and re-selects the probe, then retries a pending alert.
- **Modem bring-up (1 s interval)**: the `sim800l` component is held off the UART (`stop_poller` + `disable_loop`) while the lambda finds the baud rate and waits for network registration (`modem_stage` 0 → 1 → 2), then hands over.
- **Incoming call** (`reply_to_call`): hang up, wait until the call has ended, then send the SMS.
- **Display**: `display_wake` / `display_sleep` scripts; the display poller is stopped while the OLED is dark.

## Constraints that are easy to break

- **SMS text is temperature and status only** (`21.5C NORMAL`, or `DISCONNECTED` with no reading), for alerts and call replies alike. Both go through the `send_sms` script.
- **Modem UART is software serial** (RX GPIO15, TX GPIO13 are not the hardware UART pins). This board's modem answers at 19200, which is also the highest rate the edge-decoded software serial handles.
- **`sim800l` `update_interval` must stay at 30 s or more.** The component gives up on a command and re-sends a pending SMS after three polls; a SIM800 can take up to 60 s to confirm an SMS.
- **Never start an SMS while a call is ringing.** Each `RING` resets the component's state machine mid-exchange. `ATH` only hangs up after `AT+CVHU=0` (sent during bring-up); `AT+CHUP` is the fallback.
- **The daily SMS counter must reach flash before the SMS is sent** (`restore_from_flash: true`, then `alert_day_count->update()` and `global_preferences->sync()`), so a brownout during transmission cannot reset the budget.
- **Lambdas call ESPHome internals** that are not part of its YAML API: `id(modem).update()/disable_loop()/enable_loop()`, `id(temp).set_address(0)/setup()`, `id(onewire_bus).search()`, `alert_day_count->update()`. When changing the pinned ESPHome version, re-check these against the component sources.
- **OLED layout**: rows 0–15 are the yellow band of a two-colour panel, so the header must stay inside them; text is placed with `TextAlign::BASELINE_LEFT`. The fonts only include the glyphs listed under `font:`, so new characters must be added there.
- **I2C runs at 400 kHz** because at the default rate one OLED frame blocks the main loop for about 300 ms.
- The SIM must have its PIN disabled; `sim800l` cannot unlock one.

## Debugging on the device

The debug web UI and `make esphome-debug-logs` use the same log stream, which drops lines when many arrive at once. USB logs are complete. Logs and OTA over WiFi require joining the device's own hotspot (`ap_ssid` / `ap_password` in `secrets.yaml`).
