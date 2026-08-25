# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

Single-sketch Arduino project for a Wemos D1 Mini (ESP8266) weather station. It fetches current
conditions and a 3-day forecast from the free Open-Meteo API and renders them on SPI SH1106 OLED
displays (128x64). All logic — WiFi provisioning, HTTPS fetch/JSON parsing, and display rendering —
lives in one `.ino` file; there is no separate library/module structure.

The repo currently contains only the 2-display variant (`wather-station-2dis/`). `DOKUMENTACE.txt`
(Czech-language docs) also describes a 1-display variant (`wather-station-1dis/wather-station-1dis.ino`)
that is not present in this checkout — don't assume it exists without checking.

## Build / upload (Arduino IDE — no CLI build system)

There is no Makefile, CMake, or CLI build script; this is developed and flashed via the Arduino IDE.

- Board: ESP8266 Boards -> "LOLIN(WEMOS) D1 R2 & mini" (FQBN `esp8266:esp8266:d1_mini`, per `build/esp8266.esp8266.d1_mini/`)
- Required libraries (Arduino Library Manager): `U8g2`, `ArduinoJson`, `WiFiManager` (by tzapu), `NTPClient`
- Serial Monitor: 115200 baud — used for all runtime diagnostics (WiFi connect, HTTP status, JSON errors)
- There are no automated tests, linters, or CI in this repo. Verification is: compile in Arduino IDE,
  flash to hardware, and observe the OLED output / Serial Monitor.

## Architecture (`wather-station-2dis/wather-station-2dis.ino`)

**Location config** lives at the top of the file as plain constants (`LOCATION_NAME`, `LAT`, `LON`,
`TIMEZONE`) — this is the only thing a user is expected to edit before flashing.

**Two independent SH1106 displays** driven over a shared software SPI bus (CLK=D5, DATA=D7, DC=D3),
but each display has its **own CS and RESET pin** (disp1: CS=D2/RST=D1, disp2: CS=D6/RST=D4) — this is
a hardware requirement called out repeatedly in `DOKUMENTACE.txt` (a shared RESET line leaves the
second display dark). D8 (GPIO15) must never be used for these signals because of its pull-down resistor.

**Data flow**: `updateWeather()` does a single HTTPS GET to `api.open-meteo.com` (insecure TLS via
`BearSSL::WiFiClientSecure::setInsecure()`, no cert pinning), parses the response into global state
(`currentTemp`, `weatherCode`, `humidity`, `windSpeed`, `windDir`, `precipProb`,
`forecast{Min,Max,Code,Precip,Wind}[2]`), and sets `dataValid`/`lastError`. All rendering functions read
from this global state rather than being passed data directly.

**Rendering** is split by display and by screen: `disp1_drawCurrent`/`disp1_drawForecast` and
`disp2_drawDetails`/`disp2_drawForecast`. `loop()` toggles a `showForecast` flag every 8 seconds and
redraws both displays each iteration (`clearBuffer()` -> draw -> `sendBuffer()`), so display code always
assumes it's being called from a tight polling loop, not event-driven.

**Weather icons** are hand-authored XBM bitmaps (`icon_*[] PROGMEM`, LSB-first) at 24x24 (main) and
16x16 (small) for sun/partial-cloud/cloud/rain/snow/storm, plus small WiFi-signal-strength and
wind/rain-drop glyphs. `getWeatherIcon()`/`getWeatherIconSmall()` map Open-Meteo's WMO weather codes to
these bitmaps; `getWeatherStatus()`/`getWeatherShort()` map the same codes to Czech-language labels.
When adding support for a new weather code, update all four mapping functions together.

**Timing model** (all in `loop()`, non-blocking, `millis()`-based): weather refetch every 15 min
(900000 ms), screen swap every 8 s, plus a 100 ms delay per loop iteration. NTP/timezone sync
(`configTime` with the POSIX TZ string, for automatic CEST/CET DST) happens once in `setup()`.

**WiFi provisioning** uses `WiFiManager` with AP name `MeteoStation_AP` and a 180s config portal
timeout; on failure the device shows an error on disp1 and calls `ESP.restart()`.
