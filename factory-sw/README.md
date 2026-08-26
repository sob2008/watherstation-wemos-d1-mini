# factory-sw

Tovární firmware pro nové (nebo vrácené/refurbované) kusy meteostanice. Nahrává se **místo**
`wather-station-2dis` přes USB, a jeho jediný úkol je:

1. Provést zákazníka připojením k jeho domácí WiFi, zadáním jeho obce/města a volbou jazyka
   displeje (stejný `WiFiManager` portál `MeteoStation_AP` jako ostrý firmware, dvě pole navíc -
   viz [`LocationConfig.h`](LocationConfig.h), [`Lang.h`](Lang.h) a [`../README.md`](../README.md),
   sekce "Nastavení lokality" a "Jazyk zobrazení").
2. Hned po připojení si samo stáhnout a nainstalovat nejnovější kompatibilní verzi ostrého firmware
   z GitHub Releases (viz [`../README.md`](../README.md), sekce OTA) a restartovat se do něj.

Od tohoto restartu už zařízení běží normálně jako `wather-station-2dis` a `factory-sw` se tím
přepíše - nikdy neběží znovu (dokud by se přes USB znovu nenahrálo, např. při reklamaci). Zadaná
lokalita ale zůstává - ukládá se do LittleFS, kterou OTA/USB reflash aplikační kódu nemaže.

Používá **stejný OTA klient** (`OtaManager`/`OtaState`/`OtaVersion`/`Sha256`), **stejný modul pro
lokalitu** (`LocationConfig`) a **stejný jazykový modul** (`Lang`) jako ostrý firmware - soubory jsou
sem jen zkopírované, protože Arduino kompiluje každý sketch (složku) samostatně a cross-folder
`#include` není možný. **Pokud se tyto sdílené moduly v `wather-station-2dis/` opraví nebo změní,
zkopírujte změnu i sem** (`OtaManager.*`, `OtaState.*`, `OtaVersion.*`, `Sha256.*`,
`LocationConfig.*`, `Lang.*` - ne `OtaConfig.h`, ten je záměrně jiný, viz níže).

## Než poprvé použijete

`factory-sw` nainstaluje **existující** GitHub Release - pokud repozitář ještě žádný nemá
(viz [`../scripts/release.ps1`](../scripts/release.ps1)), zařízení bude jen dokola zkoušet a
nic se nenainstaluje (uvidíte to v Serial Monitoru jako `[OTA] Release not found`). Vydejte
alespoň jednu verzi ostrého firmware jako první.

`factory-sw/OtaConfig.h` musí mít stejné `FIRMWARE_TARGET`, `GITHUB_OWNER` a `GITHUB_REPOSITORY`
jako `wather-station-2dis/OtaConfig.h` - pokud se tyto změní tam, změňte je i tady.

## Postup - flashnutí nového kusu

1. **Zkompilovat** (v tomto adresáři `factory-sw/`, ne v podadresáři `!flash`):
   ```
   arduino-cli compile --fqbn esp8266:esp8266:d1_mini --export-binaries factory-sw
   ```
   nebo v Arduino IDE otevřít `factory-sw/factory-sw.ino` a `Sketch -> Export compiled Binary`.
2. Zkopírovat výsledný `factory-sw.ino.bin` (z `factory-sw/build/esp8266.esp8266.d1_mini/`) do
   `factory-sw/!flash/bin/`. Tahle složka je sdílená s hlavním projektem (`wather-station-2dis`) -
   pokud tam zůstane `.bin` z obou, skript při spuštění nabídne výběr, který nahrát. Pro jistotu
   je ale nejbezpečnější mít ve `bin/` vždy jen ten soubor, který chcete zrovna flashnout.
3. Připojit zařízení přes USB a spustit flash skript ze složky `!flash`:
   - Windows: `python flash.py`
   - Linux/macOS: `./flash.sh`

   Skript sám smaže flash a nahraje `factory-sw`.
4. Pro sledování průběhu (uvidíte přesně to, co vidí zákazník, a hlavně technické `[OTA] ...`
   hlášky, kdyby něco selhalo) spusťte zvlášť sériový monitor (115200 baud):
   - Windows: `python flash.py --monitor`
   - Linux/macOS: `./flash.sh --monitor`
5. Zařízení nechte zapnuté, dokud se samo nerestartuje do ostrého firmware (v monitoru uvidíte
   `[OTA] Rebooting...`). Pak je připravené k odeslání zákazníkovi - zákazník ho jen zapojí a
   projde stejným WiFi-setup krokem, který jste právě viděli.

## Co vidí zákazník / technik

Displej 1 (hlavní) postupně ukazuje: úvodní obrazovku, instrukci "Připojte se na WiFi:
MeteoStation_AP" (portál obsahuje i pole "Vaše obec/město" s nápovědou a "Jazyk / Language
(cs/en)"), po připojení IP adresu, výsledek hledání lokality, a nakonec stav instalace aktuálního
softwaru. Displej 2 doplňuje jednoduché sekundární hlášky. Od chvíle, kdy se v portálu odešle
jazyk, se všechny další obrazovky (hledání lokality, instalace) zobrazují v cs/en podle volby -
úvodní obrazovky před tím jsou vždy česky (jazyk se nezná, dokud portál nepotvrdíte). Sériová linka
(115200 baud) loguje vždy česky, technicky detailně, včetně všech `[OTA] ...` a `[LOC] ...` hlášek
popsaných v [`../README.md`](../README.md).

Pokud instalace selže (např. dočasný výpadek WiFi/GitHubu), zařízení to zkouší znovu každých 20 s
(`OTA_CHECK_INTERVAL_MS` v `factory-sw/OtaConfig.h` - záměrně mnohem kratší než 30 min u ostrého
firmware, protože u tohohle kroku stojí technik u zařízení a čeká).
