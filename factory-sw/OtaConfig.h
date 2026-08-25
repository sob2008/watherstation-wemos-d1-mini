#pragma once
// ============================================================
// OTA KONFIGURACE PRO TOVARNI SOFTWARE.
// ============================================================
// Tento soubor musi zustat v souladu s wather-station-2dis/OtaConfig.h
// v techto polozkach: FIRMWARE_TARGET, GITHUB_OWNER, GITHUB_REPOSITORY,
// OTA_ASSET_*. Pokud se nektera z nich v ostrem firmware zmeni, zmente ji
// i tady - jinak tovarni SW nenajde/neoveri spravny Release.
//
// FIRMWARE_VERSION je zde umyslne "0.0.0" - musi byt vzdy nizsi nez
// jakakoliv realna vydana verze, aby tovarni SW pri prvnim pripojeni k WiFi
// okamzite nasel a nainstaloval nejnovejsi dostupny Release (viz
// OtaManager::checkAndUpdate - "cmp <= 0" => "No update available", tohle
// zajisti, ze cmp je vzdy > 0).

// --- Identita "verze" tovarniho SW (viz komentar vyse) ---
#define FIRMWARE_VERSION "0.0.0"

// Musi presne odpovidat FIRMWARE_TARGET v wather-station-2dis/OtaConfig.h -
// tovarni SW bezi na identickem hardwaru a ma se zmenit prave na tento cil.
#define FIRMWARE_TARGET "esp8266-d1mini-wather-station-2dis"

// --- GitHub repozitar s Releases (musi odpovidat ostremu firmware) ---
#define GITHUB_OWNER "sob2008"
#define GITHUB_REPOSITORY "watherstation-wemos-d1-mini"

// Nazvy ocekavanych assetu v GitHub Release - stejne jako v ostrem firmware.
#define OTA_ASSET_FIRMWARE "firmware.bin"
#define OTA_ASSET_METADATA "firmware.json"
#define OTA_ASSET_CHECKSUM "firmware.bin.sha256"

// --- Chovani OTA ---

#define OTA_ENABLED true

// U tovarniho SW umyslne KRATCI interval nez v ostrem firmware (tam 30 min) -
// technik stoji u zarizeni a sleduje Serial Monitor, chceme rychle opakovani
// pri docasnem vypadku WiFi/GitHubu, ne cekat 30 minut.
#define OTA_CHECK_INTERVAL_MS (20UL * 1000UL) // 20 sekund

#define OTA_CONNECT_TIMEOUT_MS 10000UL
#define OTA_DOWNLOAD_TIMEOUT_MS 120000UL

// Stejna bezpecnostni ocekavani jako ostry firmware - tovarni priprava neni
// duvod checksum vyzadovat mene prisne.
#define OTA_REQUIRE_CHECKSUM true

// Tovarni SW se sam nikdy neboot uje opakovane jako "pending validation"
// (po uspesnem OTA rovnou restartuje do ostreho firmware), takze tato
// hodnota zde nema prakticky vyznam - ponechana pro konzistenci s OtaState.
#define OTA_MAX_BOOT_ATTEMPTS 3
