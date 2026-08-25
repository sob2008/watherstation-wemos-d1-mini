# wather-station-wemos-d1-mini

Meteostanice na Wemos D1 Mini (ESP8266) se dvěma SH1106 OLED displeji. Zobrazuje
aktuální počasí a předpověď z Open-Meteo.com. Podrobný popis hardwaru, zapojení
a funkcí je v [`wather-station-2dis/DOKUMENTACE.txt`](wather-station-2dis/DOKUMENTACE.txt).

Firmware podporuje aktualizaci přes OTA (over-the-air) pomocí GitHub Releases -
viz níže.

Pro tovární přípravu nových/vrácených kusů (WiFi setup zákazníka + rovnou instalace aktuálního
firmware) slouží samostatný sketch [`factory-sw/`](factory-sw/README.md).

## Vývoj

Sketch se vyvíjí a nahrává přes Arduino IDE (žádný PlatformIO):

- Deska: `Tools -> Board -> ESP8266 Boards -> LOLIN(WEMOS) D1 R2 & mini`
- Flash Size: `4MB (FS:2MB OTA:~1019KB)` (`eesz=4M2M`) - **musí zůstat toto
  nastavení**, OTA systém na něm závisí (viz níže)
- Potřebné knihovny (Library Manager): `U8g2`, `ArduinoJson` (v7), `WiFiManager`
  (tzapu), `NTPClient`
- Serial Monitor: 115200 baud - veškerá diagnostika (WiFi, HTTP, OTA) se loguje sem

## OTA aktualizace přes GitHub Releases

### Jak to funguje (přehled)

Firmware si jednou za `OTA_CHECK_INTERVAL_MS` (výchozí 30 minut) sám ověří,
jestli GitHub repozitář obsahuje novější kompatibilní Release. Pokud ano,
stáhne `firmware.bin`, ověří jeho integritu (SHA-256), velikost a cílovou
platformu, zapíše ho do volné OTA oblasti flash paměti a restartuje se.

Build a vytvoření Release zajišťuje GitHub Actions workflow
([`.github/workflows/release.yml`](.github/workflows/release.yml)), ale
spouští se **výhradně ručním pushnutím tagu** `vX.Y.Z` - žádný automatický
build při běžném push do `main`, žádné automatické zvyšování verze ani
automatické tagování. Podrobný postup níže.

Zařízení funguje plně i bez internetu/GitHubu - OTA je jen doplňková funkce,
nikdy není podmínkou pro boot ani běžný provoz.

### Flash a OTA oblasti (ESP8266, ne ESP32 - důležitý rozdíl)

Tento projekt běží na **ESP8266** (ESP8266EX), ne ESP32. ESP8266 Arduino core
nemá ESP32-styl nezávislých A/B partition s okamžitým přepnutím bootovací
partition (`esp_ota_set_boot_partition`). Místo toho používá mechanismus
"eboot": nový firmware se zapíše do volné (aktuálně nepoužívané) aplikační
oblasti flash, a teprve při následujícím restartu malý bootloader (eboot)
tuto oblast zkopíruje na místo aktuálně běžícího firmware. Prakticky to
znamená:

- Zápis nového firmware **nikdy nepřepisuje** aktuálně běžící firmware -
  probíhá do druhé, volné oblasti. Výpadek napájení během stahování/zápisu
  je tedy bezpečný, současný firmware zůstává nedotčen.
- Samotné "přepnutí" (kopírování eboot bootloaderem při dalším startu) je
  krátká operace (řádově ~1s) prováděná mimo aplikační kód - to je jediné
  reziduální riziko, které nelze z úrovně sketch/aplikace zcela odstranit
  (šlo by jen náhradou eboot samotného, což je mimo rozsah tohoto projektu).
- ESP8266 core **nemá nativní automatický rollback** jako ESP32
  (`esp_ota_mark_app_invalid_rollback_and_reboot`). Rollback je proto v tomto
  projektu implementován na aplikační úrovni (viz sekce Rollback níže).

Aktuální (výchozí, nutné zachovat) rozložení flash pro desku D1 Mini v tomto
projektu, dle Arduino IDE Flash Size `4MB (FS:2MB OTA:~1019KB)`:

| Oblast                        | Velikost              |
|--------------------------------|------------------------|
| Celková flash                  | 4 MB                   |
| LittleFS (filesystem)          | 2 MB                   |
| Aplikační OTA oblast (každá)   | **1 048 576 B (~1019 KB)** |

Tyto dvě ~1019KB oblasti fungují jako ping-pong: v jedné běží aktuální
firmware, do druhé se zapisuje nový. **Maximální velikost `firmware.bin` je
tedy 1 048 576 bajtů.** Aktuální firmware (s OTA modulem) má cca 500 KB
(511 940 B, ověřeno reálnou kompilací), tj. zhruba 49 % této kapacity - je
stále dost prostoru pro budoucí růst.

LittleFS (2 MB) se používá pro perzistentní stav OTA (viz níže) - žádná OTA
akce filesystem nemaže ani needituje mimo vlastní `/ota/` adresář.

### Konfigurace

Vše na jednom místě: [`wather-station-2dis/OtaConfig.h`](wather-station-2dis/OtaConfig.h).

- `FIRMWARE_VERSION` - verze aktuálního firmware (Semantic Versioning, viz níže)
- `FIRMWARE_TARGET` - identifikátor HW/SW varianty, kontroluje se proti
  `firmware.json` v Release (viz "Kompatibilita hardwaru")
- `GITHUB_OWNER`, `GITHUB_REPOSITORY` - **je nutné vyplnit před prvním použitím OTA**
- `OTA_ENABLED`, `OTA_CHECK_INTERVAL_MS`, timeouty, `OTA_REQUIRE_CHECKSUM`,
  `OTA_MAX_BOOT_ATTEMPTS` - viz komentáře přímo v souboru

Žádné tajné údaje (token, heslo) se nikam neukládají - repozitář je veřejný,
používá se anonymní GitHub REST API (limit 60 požadavků/hod na IP adresu;
při výchozím intervalu 30 min se spotřebovávají cca 2/hod).

### Verzování

Semantic Versioning (`MAJOR.MINOR.PATCH`), např. `1.0.0`, `1.2.3`. GitHub
Release tag může mít volitelnou předponu `v` (`v1.2.0`) - firmware ji sám
odstraní při porovnávání. Verze se porovnávají **numericky po jednotlivých
číslech**, ne lexikograficky - `1.10.0` je správně vyhodnoceno jako novější
než `1.9.0` (viz `wather-station-2dis/OtaVersion.h`, ověřeno testy v
`tests/test_ota_version.cpp`). Firmware nikdy automaticky "downgraduje" -
pokud je Release stejný nebo starší než aktuální verze, OTA se přeskočí.

### Formát GitHub Release

```
Tag:  v1.2.0        (musí být parsovatelný jako MAJOR.MINOR.PATCH)

Assets:
  firmware.bin        - povinný, zkompilovaný .bin pro tuto desku
  firmware.json        - doporučený, viz níže
  firmware.bin.sha256   - volitelný fallback, pokud není firmware.json
```

**`firmware.json`** (doporučeno) - malý JSON soubor, který dovoluje ověřit
cíl a integritu ještě před/bez nutnosti samostatného `.sha256` souboru:

```json
{
  "version": "1.2.0",
  "target": "esp8266-d1mini-wather-station-2dis",
  "firmware_size": 512430,
  "sha256": "1a2b3c...64 hex znaku..."
}
```

`target` musí přesně odpovídat `FIRMWARE_TARGET` v `OtaConfig.h`, jinak OTA
odmítne firmware nainstalovat (`[OTA] ERROR: Incompatible firmware`).

Pokud `firmware.json` v Release chybí, firmware se pokusí najít
`firmware.bin.sha256` (obsahuje buď samotný 64znakový hex SHA-256, nebo
formát `sha256sum` - `hash  firmware.bin`). Kontrola cíle (target) bez
`firmware.json` možná není - loguje se varování.

### Release workflow (automatizovaný přes GitHub Actions)

Build i vytvoření Release zajišťuje [`.github/workflows/release.yml`](.github/workflows/release.yml).
Spouští se **výhradně ručním pushnutím tagu** - žádný push do `main` ani jiná
událost release nevytvoří.

**Nejjednodušší způsob - jeden skript:**

```powershell
.\scripts\release.ps1 -Version 1.2.0
```

Skript ([`scripts/release.ps1`](scripts/release.ps1)) nastaví
`FIRMWARE_VERSION` v `OtaConfig.h`, vytvoří commit a tag `v1.2.0`, ukáže
shrnutí a před finálním `git push` se ještě zeptá na potvrzení. Po potvrzení
už jen sleduješ záložku *Actions* na GitHubu - zbytek (kompilace, SHA-256,
`firmware.json`, GitHub Release) udělá CI sám.

**Co se děje na pozadí (pokud chcete kroky ručně, bez skriptu):**

1. Upravit firmware.
2. Zvýšit `FIRMWARE_VERSION` v `OtaConfig.h` (Semantic Versioning).
3. Commitnout a pushnout změnu do `main`.
4. Vytvořit a pushnout tag **odpovídající `FIRMWARE_VERSION`** (workflow to
   ověřuje a při neshodě selže, aniž by cokoliv publikoval):
   ```
   git tag v1.2.0
   git push origin v1.2.0
   ```
5. GitHub Actions runner zkompiluje sketch přes `arduino-cli` proti stejné
   desce (`esp8266:esp8266:d1_mini`) a stejným knihovnám jako lokální vývoj,
   spočítá SHA-256, vygeneruje `firmware.json` + `firmware.bin.sha256` a
   vytvoří GitHub Release s tagem `vX.Y.Z` a všemi třemi assety.
6. Zařízení při nejbližší pravidelné kontrole (do `OTA_CHECK_INTERVAL_MS`)
   novou verzi zjistí, ověří a nainstaluje automaticky.

Pokud kompilace selže nebo verze v tagu nesouhlasí s `FIRMWARE_VERSION`,
workflow skončí chybou a **žádný Release nevznikne** - není možné omylem
publikovat nezkompilovatelný nebo špatně označený firmware. Krok 1 (úprava
firmware) skript samozřejmě neumí - to je jediná ruční část, která zbývá.

**Ruční alternativa** (bez CI, např. pro lokální testování před tagem):
zkompilovat (`arduino-cli compile --fqbn esp8266:esp8266:d1_mini
wather-station-2dis --export-binaries` nebo Arduino IDE `Sketch -> Export
compiled Binary`), výsledný `.bin` přejmenovat na `firmware.bin`, spočítat
SHA-256 (`sha256sum firmware.bin` / `Get-FileHash -Algorithm SHA256`), ručně
sestavit `firmware.json` dle vzoru výše a přidat všechny tři soubory jako
assety k ručně vytvořenému Release.

### Kontrola velikosti

Před stažením se porovná deklarovaná velikost (z `firmware.json`, nebo
velikost GitHub asset záznamu) s volnou OTA oblastí (`ESP.getFreeSketchSpace()`,
1 048 576 B). Pokud je firmware větší, OTA se **zruší bez jakéhokoli zápisu do
flash** (`[OTA] ERROR: Firmware too large` / `[OTA] Update cancelled`).

Protože se podle zadání nemá spoléhat jen na `Content-Length`/deklarovanou
velikost, je při streamovaném zápisu navíc průběžně kontrolován návratový
kód `Update.write()` - jakmile by zápis překročil skutečně volné místo,
zápis se okamžitě přeruší a OTA se bezpečně zruší (aktuální firmware
zůstává nedotčen, protože se zapisovalo do volné, nepoužívané oblasti).

### Integrita firmware (SHA-256) a TLS - důležité rozhodnutí

**HTTPS spojení pro OTA (GitHub API i stahování `firmware.bin`) používá
`setInsecure()`** - stejně jako už dnes existující kód projektu pro
Open-Meteo API. Toto NENÍ přehlédnutí: skutečné ověření TLS certifikátu
(pinning kořenové CA) bylo zvažováno, ale během vývoje nebylo možné
spolehlivě získat a ověřit reálné kořenové certifikáty GitHubu
(`api.github.com` a CDN pro assety používají různé certifikační autority a
prostředí použité pro jejich stažení je nedůvěryhodné pro tento účel).
Zabudování špatného/neověřeného certifikátu do firmware by OTA trvale a
tiše rozbilo - to je horší než zdokumentované riziko `setInsecure()`.

Jako kompenzaci je proto v tomto projektu **SHA-256 kontrola integrity
POVINNÁ** (`OTA_REQUIRE_CHECKSUM = true` ve výchozím nastavení, na rozdíl od
zadání, kde je volitelná) - bez platného checksumu ve `firmware.json` nebo
`firmware.bin.sha256` se OTA vždy zruší. To je hlavní ochrana proti
poškozenému/neúmyslně změněnému obsahu při přenosu.

**Pokud budete chtít v budoucnu doplnit skutečné TLS pinning**: ESP8266
BearSSL (`WiFiClientSecureBearSSL.h`) podporuje `setTrustAnchors()` s
vlastním `BearSSL::X509List` obsahujícím kořenové certifikáty - stačí je
vygenerovat z ověřeného zdroje (např. přímo z prohlížeče/`openssl s_client`
na důvěryhodném počítači, ne v sandboxu) a nahradit `setInsecure()` volání v
`OtaManager.cpp`. Pozor: `api.github.com`/`github.com` a CDN pro stahování
assetů (`objects.githubusercontent.com`) mohou být na různých kořenových CA -
je nutné připnout obě.

### Kompatibilita hardwaru

`FIRMWARE_TARGET` v `OtaConfig.h` (výchozí
`esp8266-d1mini-wather-station-2dis`) se porovnává s polem `target` ve
`firmware.json`. Při neshodě se OTA odmítne
(`[OTA] ERROR: Incompatible firmware`) - chrání to před nahráním firmware
určeného pro jinou variantu HW (např. verzi s jedním displejem), pokud by
oba typy sdílely stejný GitHub repozitář.

### Rollback a ochrana proti nekonečné smyčce

Protože ESP8266 nemá nativní rollback, je implementován takto:

1. Před dokončením každé úspěšné OTA se stažený (a SHA-256 ověřený) obsah
   `firmware.bin` zároveň průběžně ukládá do `/ota/candidate.bin` na LittleFS.
2. Po úspěšném zápisu (`Update.end()`) se do `/ota/state.json` zapíše
   "pending_validation = true" pro danou verzi a zařízení se restartuje.
3. Po startu nového firmware se počítadlo pokusů o boot uloží na flash
   (přežije i výpadek napájení). Jakmile hlavní aplikace prokáže, že funguje
   (WiFi připojeno + proběhl první cyklus hlavní funkce), firmware se označí
   za ověřený - `/ota/candidate.bin` se povýší na `/ota/last_good.bin`
   (nová záloha pro případný budoucí rollback) a počítadlo se vynuluje.
4. Pokud se firmware **opakovaně nedostane k tomuto potvrzení** (spadne/
   resetuje se watchdogem dřív, než doběhne `setup()`) víc než
   `OTA_MAX_BOOT_ATTEMPTS`-krát (výchozí 3×) po sobě, zařízení automaticky
   přeflashuje `/ota/last_good.bin` a restartuje se zpět do poslední funkční
   verze.
5. Verze, která takto selhala, se zapíše jako "last_failed_version" -
   při dalších pravidelných kontrolách se **stejná** verze znovu
   nestahuje/nezkouší, dokud nevyjde novější Release (nebo dokud se stav
   ručně nevymaže, viz níže).

**Důležité omezení**: záloha (`last_good.bin`) existuje až od **druhé**
úspěšné OTA aktualizace. Úplně první OTA (z firmware nahraného přes USB) nemá
co zálohovat - pokud by tato první OTA byla vadná, automatický rollback není
možný a je nutný ruční zásah (nahrání přes USB). Od druhé OTA výše je
rollback plně automatický.

**Detekce "stale" stavu**: `pending_validation` se navíc porovnává s
`FIRMWARE_VERSION` skutečně běžícího firmware - pokud dojde k výpadku
napájení přesně mezi zápisem tohoto stavu a provedením eboot swapu, další
boot to sám rozpozná (verze nesouhlasí) a stav bezpečně vyčistí, místo
falešné validace nebo falešného rollbacku.

**Ruční reset stavu** (např. chcete znovu zkusit verzi označenou jako
"failed"): smažte `/ota/` na zařízení (lze např. dočasně přidat sketch,
který zavolá `LittleFS.format()`, nebo počkat na vydání novější verze -
ta se vždy zkusí bez ohledu na předchozí selhání).

### Watchdog a OTA interval

Stahování/zápis firmware běží v hlavní smyčce (`loop()`), stejně jako dnešní
stahování počasí - během čtení síťových dat se pravidelně volá `delay(0)`/
`yield()`, aby nedošlo k resetu SDK watchdogem. Celé stahování má navíc
vlastní timeout (`OTA_DOWNLOAD_TIMEOUT_MS`, výchozí 120 s) - při překročení
se OTA bezpečně zruší. Kontrola GitHub Releases neběží v každém `loop()`
průchodu, jen jednou za `OTA_CHECK_INTERVAL_MS` (výchozí 30 minut).

### Bezpečný stav zařízení

Tento projekt neovládá žádné relé/motory/topení - jediný "výstup" jsou dva
OLED displeje. Bezpečný stav proto znamená jen: během aktivního
stahování/zápisu firmware se na hlavním displeji zobrazuje stavová hláška
(`OtaManager::setStatusCallback`), aby displej nepůsobil zamrzle; žádná jiná
zvláštní příprava není potřeba.

### Chování v okrajových situacích

| Situace | Chování |
|---|---|
| Není internet / WiFi | OTA kontrola se přeskočí (`[OTA] Skipping check: WiFi not connected`), zařízení pokračuje normálně |
| GitHub nedostupný | `[OTA] GitHub unavailable`, pokračuje normálně |
| Release neexistuje | `[OTA] Release not found`, pokračuje normálně |
| Release nemá `firmware.bin` | `[OTA] Firmware asset not found`, pokračuje normálně |
| Release má stejnou/starší verzi | `[OTA] No update available` |
| Release již dříve selhal | `[OTA] Firmware previously failed`, čeká na novější verzi |
| Firmware je pro jiný target | `[OTA] ERROR: Incompatible firmware`, zrušeno |
| Firmware je větší než OTA oblast | `[OTA] ERROR: Firmware too large`, zrušeno bez zápisu |
| Chybí SHA-256 (a je vyžadován) | `[OTA] ERROR: No checksum available (required)`, zrušeno |
| SHA-256 nesouhlasí | `[OTA] ERROR: Checksum mismatch`, zrušeno, žádný swap se neprovede |
| Stahování se přeruší/timeoutne | OTA zrušeno, současný firmware nedotčen |
| Výpadek napájení během stahování | Bezpečné - zapisuje se do volné (neaktivní) OTA oblasti |
| Výpadek napájení během eboot swapu | Jediné neodstranitelné reziduální riziko, viz sekce "Flash a OTA oblasti" |
| Nový firmware opakovaně nenaboot uje | Automatický rollback na `last_good.bin` po `OTA_MAX_BOOT_ATTEMPTS` pokusech |
| Rollback proveden | Verze se zapíše jako "failed", nezkouší se znovu dokola |

## Testy

Logika, která nezávisí na Arduino/ESP8266 API (SHA-256, porovnávání verzí),
je pokrytá host-side testy mimo sketch, spustitelnými běžným g++:

```
g++ -std=c++17 -Wall -Wextra -o test_sha256.exe tests/test_sha256.cpp wather-station-2dis/Sha256.cpp
g++ -std=c++17 -Wall -Wextra -o test_ota_version.exe tests/test_ota_version.cpp wather-station-2dis/OtaVersion.cpp
```

Zbytek OTA modulů (`OtaState`, `OtaManager` - LittleFS, HTTPClient, Update)
závisí na ESP8266 core a je ověřen reálnou kompilací (`arduino-cli compile
--fqbn esp8266:esp8266:d1_mini --warnings all wather-station-2dis`, 0 chyb,
0 varování), ne spuštěním. **Co nebylo a nemůže být ověřeno bez fyzického
zařízení**: skutečný WiFi/HTTPS provoz, reálný eboot swap při restartu,
chování watchdogu při dlouhém stahování, a celý rollback cyklus (vyžaduje
záměrně vadný Release a sledování chování přes několik rebootů).
