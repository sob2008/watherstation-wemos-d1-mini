#pragma once
// ============================================================
// OTA KONFIGURACE - jedine misto pro nastaveni OTA systemu.
// ============================================================
// Zadne tajne udaje (GitHub token, hesla, klice) sem NEPATRI.
// Repozitar je verejny, pouziva se anonymni GitHub REST API.
// Podrobny popis rucniho Release workflow: viz README.md v korenu projektu.

// --- Verze a identita tohoto firmware ---

// Zvysujte pri kazde zmene, kterou chcete distribuovat pres OTA.
// Pouziva se Semantic Versioning (MAJOR.MINOR.PATCH), viz OtaVersion.h.
#define FIRMWARE_VERSION "1.0.6"

// Identifikuje HW/SW variantu tohoto firmware. OTA odmitne nainstalovat
// release, jehoz firmware.json obsahuje jiny "target" (viz README, sekce
// "Kompatibilita hardwaru").
#define FIRMWARE_TARGET "esp8266-d1mini-wather-station-2dis"

// --- GitHub repozitar s Releases ---
#define GITHUB_OWNER "sob2008"
#define GITHUB_REPOSITORY "watherstation-wemos-d1-mini"

// Nazvy ocekavanych assetu v GitHub Release.
// firmware.json je volitelny, ale doporuceny - pokud je pritomen, pouzije
// se pro kontrolu "target" a SHA-256 bez nutnosti samostatneho .sha256 souboru.
#define OTA_ASSET_FIRMWARE "firmware.bin"
#define OTA_ASSET_METADATA "firmware.json"
#define OTA_ASSET_CHECKSUM "firmware.bin.sha256"

// --- Chovani OTA ---

// Globalni vypinac - pri false OtaManager::handle() nic nedela.
#define OTA_ENABLED true

// Jak casto (ms) se kontroluje GitHub Releases na novou verzi.
#define OTA_CHECK_INTERVAL_MS (30UL * 60UL * 1000UL) // 30 minut

// Timeouty sitovych operaci (ms).
#define OTA_CONNECT_TIMEOUT_MS 10000UL
#define OTA_DOWNLOAD_TIMEOUT_MS 120000UL // cely stahovaci cyklus firmware.bin

// Vyzadovat platny SHA-256 checksum, jinak OTA zrusit (fail-closed).
// Vychozi true: viz README, sekce "TLS a integrita firmware" - HTTPS
// spojeni pro OTA pouziva (stejne jako zbytek projektu) setInsecure(),
// takze checksum je zde skutecnou, ne jen volitelnou pojistkou integrity.
#define OTA_REQUIRE_CHECKSUM true

// Kolik po sobe jdoucich neuspesnych bootu noveho (jeste nevalidovaneho)
// firmware je povoleno, nez OtaManager provede automaticky rollback na
// /ota/last_good.bin.
#define OTA_MAX_BOOT_ATTEMPTS 3
