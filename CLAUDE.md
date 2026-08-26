# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

Arduino project for a Wemos D1 Mini (ESP8266) weather station. It fetches current conditions and a
3-day forecast from the free Open-Meteo API, renders them on two SPI SH1106 OLED displays (128x64),
and can update its own firmware over the air (OTA) from GitHub Releases. All application logic lives in
`wather-station-2dis/wather-station-2dis.ino`; the OTA subsystem lives in a handful of sibling
`.h`/`.cpp` files in the same folder (Arduino compiles every `.h`/`.cpp` in the sketch folder together
with the `.ino`, so this is a normal multi-file sketch, not a separate library).

The repo currently contains only the 2-display variant (`wather-station-2dis/`). `DOKUMENTACE.txt`
(Czech-language docs) also describes a 1-display variant (`wather-station-1dis/wather-station-1dis.ino`)
that is not present in this checkout — don't assume it exists without checking.

There is a second, separate sketch: `factory-sw/factory-sw.ino` — a minimal provisioning firmware
flashed onto new/refurbished units via USB before shipping. It walks the customer through WiFi setup
then immediately installs the latest `wather-station-2dis` release via OTA and reboots into it; see
`factory-sw/README.md`. It duplicates the OTA client files (`Ota*.h/.cpp`, `Sha256.h/.cpp`) and the location module
(`LocationConfig.h/.cpp`) from `wather-station-2dis/` because Arduino compiles each sketch folder
independently — **if you fix or change OTA client or location logic in `wather-station-2dis/`, copy
the same change into `factory-sw/`** (but not `OtaConfig.h`, which is deliberately different between
the two — see that file's own comments).

## Build / test commands

**Firmware (Arduino IDE or arduino-cli)** — no Makefile/CMake, this is a normal Arduino sketch:

- Board: ESP8266 Boards -> "LOLIN(WEMOS) D1 R2 & mini", FQBN `esp8266:esp8266:d1_mini`
- Flash Size **must stay** `4MB (FS:2MB OTA:~1019KB)` (`eesz=4M2M`) — the OTA subsystem's size checks
  are written against this exact layout (see `wather-station-2dis/OtaConfig.h` and README.md)
- Required libraries: `U8g2`, `ArduinoJson` (v7 API), `WiFiManager` (tzapu), `NTPClient`
- CLI compile: `arduino-cli compile --fqbn esp8266:esp8266:d1_mini --warnings all wather-station-2dis`
- Serial Monitor: 115200 baud — all runtime diagnostics, OTA included, are logged with an `[OTA]` prefix

**Host-side unit tests** — the parts of the OTA subsystem with no Arduino/ESP8266 dependency
(`Sha256`, `OtaVersion`) are deliberately written to compile with a plain host g++ and are tested
outside the sketch folder in `tests/` (Arduino only compiles files inside the sketch's own folder, so
this can't accidentally get picked up by the firmware build):

```
g++ -std=c++17 -Wall -Wextra -o test_sha256.exe tests/test_sha256.cpp wather-station-2dis/Sha256.cpp
g++ -std=c++17 -Wall -Wextra -o test_ota_version.exe tests/test_ota_version.cpp wather-station-2dis/OtaVersion.cpp
```

The rest of the OTA code (`OtaState`, `OtaManager`) depends on LittleFS/HTTPClient/Update and is only
verified by real compilation against the ESP8266 core, not host-executable — see README.md's "Testy"
section for what has and hasn't been exercised on real hardware.

**Release**: `scripts/release.ps1 -Version X.Y.Z` bumps `FIRMWARE_VERSION`, commits, tags, and (after
confirmation) pushes — which triggers `.github/workflows/release.yml` to compile the firmware and
publish it as a GitHub Release. See README.md for the full flow; this is the only CI in the repo and it
only runs on an explicit `vX.Y.Z` tag push, never on a plain push to `main`.

## Architecture

### Weather station (`wather-station-2dis.ino`)

**Location** (`LOCATION_NAME`/`LAT`/`LON` equivalent) is *not* compile-time config — each customer
has a different one, entered at runtime via a custom field in the WiFiManager portal and resolved to
coordinates through the Open-Meteo Geocoding API; see "Location subsystem" below. `TIMEZONE` (a plain
constant near the top of the file) and `OtaConfig.h` are the only things still edited before flashing.

**Two independent SH1106 displays** driven over a shared software SPI bus (CLK=D5, DATA=D7, DC=D3),
but each display has its **own CS and RESET pin** (disp1: CS=D2/RST=D1, disp2: CS=D6/RST=D4) — this is
a hardware requirement called out repeatedly in `DOKUMENTACE.txt` (a shared RESET line leaves the
second display dark). D8 (GPIO15) must never be used for these signals because of its pull-down resistor.

**Data flow**: `updateWeather()` does a single HTTPS GET to `api.open-meteo.com` (insecure TLS via
`BearSSL::WiFiClientSecure::setInsecure()` — see "TLS" note below), parses the response into global
state (`currentTemp`, `weatherCode`, `humidity`, `windSpeed`, `windDir`, `precipProb`,
`forecast{Min,Max,Code,Precip,Wind}[2]`), and sets `dataValid`/`lastError`. All rendering functions read
from this global state rather than being passed data directly.

**Rendering** is split by display and by screen: `disp1_drawCurrent`/`disp1_drawForecast` and
`disp2_drawDetails`/`disp2_drawForecast`. `loop()` toggles a `showForecast` flag every 8 seconds and
redraws both displays each iteration (`clearBuffer()` -> draw -> `sendBuffer()`), so display code always
assumes it's being called from a tight polling loop, not event-driven. `otaStatusCallback()` is the one
other thing allowed to draw to `disp1`, used only while an OTA download/flash is actively in progress.

**Weather icons** are hand-authored XBM bitmaps (`icon_*[] PROGMEM`, LSB-first) at 24x24 (main) and
16x16 (small) for sun/partial-cloud/cloud/rain/snow/storm, plus small WiFi-signal-strength and
wind/rain-drop glyphs. `getWeatherIcon()`/`getWeatherIconSmall()` map Open-Meteo's WMO weather codes to
these bitmaps; `getWeatherStatus()`/`getWeatherShort()` map the same codes to Czech-language labels.
When adding support for a new weather code, update all four mapping functions together.

**Timing model** (all in `loop()`, non-blocking, `millis()`-based): weather refetch every 15 min
(900000 ms), screen swap every 8 s, OTA check every 30 min (`OTA_CHECK_INTERVAL_MS`), plus a 100 ms
delay per loop iteration. NTP/timezone sync (`configTime` with the POSIX TZ string, for automatic
CEST/CET DST) happens once in `setup()`.

**WiFi provisioning** uses `WiFiManager` with AP name `MeteoStation_AP` and a 180s config portal
timeout; on failure the device shows an error on disp1 and calls `ESP.restart()`. The same portal
carries one extra field (a `WiFiManagerParameter`) for the customer's town/city — see "Location
subsystem". A double physical RESET within `DOUBLE_RESET_WINDOW_MS` (~2s, detected via ESP8266 RTC
memory, `checkDoubleReset()`/`clearDoubleResetMarker()` in the `.ino`) calls `wm.resetSettings()` to
reopen the portal later, e.g. if the customer moves or mistypes their town.

### Location subsystem (`LocationConfig.h/.cpp`)

Per-customer location (name + lat/lon) is no longer compile-time config. It's entered as a plain
town/city name into the WiFiManager portal's custom field, then `LocationConfig::geocodeAndSave()`
resolves it via the Open-Meteo Geocoding API (`geocoding-api.open-meteo.com`, same provider as the
weather API — no new dependency) and persists it to LittleFS at `/config/location.json`
(write-temp-then-rename, same pattern as `OtaState`). `LocationConfig::name()`/`lat()`/`lon()` are
what `updateWeather()` and the display headers read instead of the old `LOCATION_NAME`/`LAT`/`LON`
constants; `name()` returns `const String&` (not a copy) since it's called on every display redraw.
Geocoding only runs when the submitted value differs from what's already stored — on a normal boot
(saved WiFi credentials, portal never shown) the field just echoes the stored value back, so nothing
re-fetches every boot. On failure (typo, no network) the previous or default (`ROJETIN`, 49.36/16.26)
location is kept — never blocks boot. Duplicated into `factory-sw/` for the same reason as the OTA
client (see below); keep both copies in sync.

The U8g2 fonts used (`6x10`/`7x14B`/`5x7`) render Czech čárka letters (á,é,í,ó,ú,ý) but not
háček/kroužek ones (č,ď,ě,ň,ř,š,ť,ů,ž) — confirmed by directly linking `u8g2_font.c`/`u8g2_fonts.c`
against a host test harness and calling `u8g2_IsGlyph()` across all ~1930 bundled fonts, not by
guessing; the smallest font that covers all of them is `unifont_t_extended` at 16x16px, too big for
this layout. So the customer-entered name goes to the geocoding query *with* diacritics (better match
accuracy) but `geocodeAndSave()` runs it through `stripDiacritics()` (byte-level UTF-8 table, tested
in `tests/test_strip_diacritics.cpp`) before storing — `LocationConfig::name()` is always
display-safe ASCII.

### Language subsystem (`Lang.h/.cpp`)

Display language (Czech default / English) is a third WiFiManager portal field alongside WiFi and
city, persisted to `/config/language.json` the same way. `LangId` is an enum covering every
user-facing display string (weather conditions long/short form, wind direction letters, and the
fixed screen text in both `.ino` files); `Lang::t(id)` returns the string in the current language
from one of two parallel `const char*` arrays whose length is checked against the enum at compile
time via `static_assert`. All English strings are ASCII-only too, for the same font reason as above.
Serial diagnostic logging is Czech-only regardless of this setting (out of scope — customer-facing
display text only). Duplicated into `factory-sw/` like the other shared modules.

### OTA subsystem (`Ota*.h/.cpp`, `Sha256.h/.cpp`)

Firmware self-updates from GitHub Releases; GitHub Actions is only used to *build and publish* those
releases (on a manually-pushed tag), never to push firmware to devices directly. Full behavioral spec —
flash/partition sizing, TLS trade-off rationale, rollback mechanics, release asset format — is in
README.md; this section is about where the code lives.

- **`OtaConfig.h`** — every tunable in one place (`FIRMWARE_VERSION`, `FIRMWARE_TARGET`, `GITHUB_OWNER`/
  `GITHUB_REPOSITORY`, intervals/timeouts, `OTA_REQUIRE_CHECKSUM`, `OTA_MAX_BOOT_ATTEMPTS`). No secrets.
- **`Sha256.h/.cpp`** — standalone SHA-256 (FIPS 180-4), no Arduino dependency on purpose so it's
  host-testable. Used to verify every downloaded firmware image against `firmware.json`/
  `firmware.bin.sha256` before it's ever booted.
- **`OtaVersion.h/.cpp`** — also Arduino-independent. Parses `vMAJOR.MINOR.PATCH` tags and compares them
  **numerically** field-by-field (`1.10.0 > 1.9.0`), not lexicographically.
- **`OtaState.h/.cpp`** — persistent state on LittleFS under `/ota/`: `state.json` (pending-validation
  flag, boot-attempt counter, last-failed version — written temp-file-then-rename to survive power
  loss mid-write), `candidate.bin` (firmware currently being validated), `last_good.bin` (last
  known-good backup, promoted from `candidate.bin` only after a successful boot). This is the module to
  read first when reasoning about rollback/boot-loop behavior.
- **`OtaManager.h/.cpp`** — orchestration: `begin()` (boot-time rollback decision, called before WiFi
  connects), `notifyApplicationHealthy()` (marks the running firmware validated), `handle()`
  (interval-gated GitHub check + blocking download/flash/verify cycle, called from `loop()`).

**ESP8266-specific caveat that shapes this whole module**: unlike ESP32, the ESP8266 Arduino core has no
native A/B partition rollback — `Update`/eboot is a copy-based ping-pong scheme, not an instant boot-
partition switch. `OtaState`'s LittleFS-backed backup/boot-attempt-counter is what actually implements
rollback at the application level; don't assume ESP32 OTA idioms apply here. See README.md's "Flash a
OTA oblasti" section before changing anything in `Update.begin()`/`Update.end()` call sites.

**TLS**: all HTTPS in this project (weather API and OTA) uses `BearSSL::WiFiClientSecure::setInsecure()`
— deliberate, not an oversight; see README.md's "Integrita firmware (SHA-256) a TLS" section before
"fixing" this. SHA-256 verification is mandatory for OTA specifically to compensate.
