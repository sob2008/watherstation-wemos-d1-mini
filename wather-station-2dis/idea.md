Implementuj do tohoto projektu robustní, bezpečný a fail-safe OTA systém pro aktualizaci firmware pomocí GitHub Releases.

TENTO PROMPT JE UNIVERZÁLNÍ.
Nejdříve celý projekt analyzuj a zjisti skutečný hardware a architekturu projektu. Nic nepředpokládej.

============================================================
DŮLEŽITÉ OMEZENÍ – GITHUB
============================================================

GitHub používám pouze jako místo pro GitHub Releases.

VŠECHNO KOLEM BUILDŮ A RELEASE BUDU DĚLAT RUČNĚ.

NEVYTVÁŘEJ:
- GitHub Actions
- automatický build
- automatickou kompilaci
- automatické vytváření Release
- automatické vytváření tagů
- automatický upload firmware
- automatické GitHub workflow

Já sám:
1. upravím firmware,
2. změním verzi,
3. ručně zkompiluji firmware,
4. vytvořím výsledný .bin,
5. vytvořím GitHub Release,
6. přidám do Release firmware asset.

ESP bude pouze:
- kontrolovat nejnovější GitHub Release,
- zjistí, zda existuje novější kompatibilní firmware,
- ověří ho,
- bezpečně ho stáhne,
- bezpečně provede OTA,
- případně provede rollback.

============================================================
1. NEJDŘÍVE ANALÝZA PROJEKTU
============================================================

NEŽ COKOLIV ZMĚNÍŠ, PROVEĎ DŮKLADNOU ANALÝZU CELÉHO PROJEKTU.

Zjisti minimálně:

- přesný typ ESP / MCU,
- variantu čipu,
- použitý framework,
- Arduino / PlatformIO / ESP-IDF / jiný systém,
- způsob buildování,
- hlavní firmware soubory,
- současný systém verzování,
- současný OTA systém, pokud existuje,
- flash size,
- partition table,
- velikost všech důležitých partition,
- OTA_0 / OTA_1,
- NVS,
- LittleFS,
- SPIFFS,
- EEPROM / emulovanou EEPROM,
- případné další persistentní oblasti,
- bootloader,
- watchdog,
- FreeRTOS tasky, pokud existují,
- Wi-Fi/network architekturu,
- současné logování,
- případné řízení relé, motorů nebo jiných výstupů,
- kritické operace, které nesmí být přerušeny.

Prozkoumej také existující konfiguraci projektu.

Pokud už projekt OTA obsahuje, nejdříve ho analyzuj a pokud je kvalitní, rozšiř ho místo vytváření druhého paralelního OTA systému.

Nenič současnou funkcionalitu.

============================================================
2. ZÁSADA: OTA NESMÍ ROZBÍT FUNKČNÍ FIRMWARE
============================================================

Toto je nejdůležitější požadavek.

OTA musí být navrženo tak, aby:

- výpadek napájení během OTA nezničil současný firmware,
- přerušené Wi-Fi nezničilo současný firmware,
- přerušené HTTPS spojení nezničilo současný firmware,
- poškozený firmware nebyl spuštěn,
- příliš velký firmware nebyl nahráván,
- nekompatibilní firmware nebyl nahrán,
- nový firmware, který po bootu selže, mohl být rollbackován,
- zařízení se nedostalo do nekonečné OTA/restart smyčky.

Aktuálně běžící firmware NESMÍ být během OTA předčasně smazán nebo přepsán.

Použij standardní A/B OTA mechanismus konkrétního ESP/frameworku, pokud ho hardware podporuje.

============================================================
3. FLASH A PARTITION LAYOUT
============================================================

Zjisti skutečnou velikost flash.

Zjisti přesné velikosti:

- bootloader,
- partition table,
- OTA_0,
- OTA_1,
- factory, pokud existuje,
- NVS,
- LittleFS,
- SPIFFS,
- ostatní partition.

Pokud současné partition scheme OTA podporuje, zachovej ho.

Pokud OTA nepodporuje, navrhni a implementuj vhodný OTA-capable layout.

Při změně partition scheme:

- zachovej NVS,
- zachovej filesystem,
- zachovej persistentní data,
- maximalizuj rozumně velikost OTA partitions,
- zachovej dvě aplikační OTA partitions,
- nezmenšuj filesystem bez důvodu.

NEPŘEDPOKLÁDEJ velikost flash.

Použij skutečnou konfiguraci projektu.

DŮLEŽITÉ:

OTA firmware musí být vždy menší nebo roven velikosti cílové OTA partition.

============================================================
4. KONTROLA VELIKOSTI FIRMWARE
============================================================

PŘED samotným OTA zápisem ověř:

firmware_size
vs.
target_ota_partition_size

Pokud:

firmware_size > target_ota_partition_size

OTA MUSÍ BÝT ZRUŠENO.

Nesmí dojít k žádnému pokusu o nebezpečný zápis.

ESP musí pokračovat ve stávajícím firmware.

Použij například log:

[OTA] Firmware size: XXXXX bytes
[OTA] OTA partition size: XXXXX bytes
[OTA] Size check: OK

nebo:

[OTA] ERROR: Firmware too large
[OTA] Update cancelled

Pokud Content-Length není dostupný nebo je nespolehlivý, nespoléhej pouze na něj. Během zápisu musí být možné bezpečně zjistit překročení dostupného prostoru a update ukončit.

============================================================
5. VERZOVÁNÍ
============================================================

Implementuj jednotný systém verzování firmware.

Pokud projekt už má systém verzování, použij ho.

Jinak vytvoř například:

FIRMWARE_VERSION = "1.0.0"

Používej Semantic Versioning:

1.0.0
1.0.1
1.1.0
2.0.0

GitHub Releases:

v1.0.0
v1.0.1
v1.1.0
v2.0.0

Porovnávej verze numericky.

Například:

1.10.0 > 1.9.0

Nepoužívej obyčejné lexikografické porovnání stringů.

Podporuj případné počáteční "v" v GitHub Release tagu.

============================================================
6. GITHUB RELEASE API
============================================================

Použij GitHub Releases API pro získání nejnovějšího Release.

Konfigurace musí obsahovat jednoznačně:

GITHUB_OWNER
GITHUB_REPOSITORY

a název firmware assetu:

firmware.bin

ESP nesmí mít natvrdo uložené číslo aktuální Release.

Musí umět zjistit aktuální nejnovější Release.

Pokud GitHub není dostupný:

- OTA přeskočit,
- zařízení pokračuje normálně.

Pokud Release neexistuje:

- OTA přeskočit,
- zařízení pokračuje normálně.

Pokud Release nemá firmware asset:

- OTA přeskočit,
- zařízení pokračuje normálně.

============================================================
7. FIRMWARE ASSET
============================================================

Standardní asset:

firmware.bin

Volitelně podporuj:

firmware.bin.sha256

Pokud je implementován checksum, ESP musí ověřit SHA-256 před označením nového firmware jako bootovatelného.

Nepoužívej firmware, jehož checksum nesedí.

============================================================
8. KOMPATIBILITA HARDWARU
============================================================

Zabraň možnosti nahrát firmware určený pro jiný hardware, pokud to architektura projektu umožňuje rozumně kontrolovat.

Implementuj identifikaci targetu firmware.

Například pomocí konstanty:

FIRMWARE_TARGET

která odpovídá konkrétnímu zařízení/boardu.

Pokud projekt obsahuje více variant hardware, musí být možné je rozlišit.

Pokud je možné ověřit:

- chip family,
- board variant,
- flash konfiguraci,
- firmware target,

proveď tuto kontrolu před OTA.

Pokud firmware není kompatibilní:

OTA ZRUŠIT.

============================================================
9. BEZPEČNOST
============================================================

Používej HTTPS.

Nepoužívej GitHub Personal Access Token, pokud není potřeba.

Pokud je repository veřejné, používej veřejné GitHub API.

Do firmware NEVKLÁDEJ:
- GitHub token,
- hesla,
- secrets,
- privátní klíče, pokud nejsou nezbytně nutné.

Pokud framework podporuje správnou TLS verifikaci certifikátu, použij ji.

Neřeš TLS bezpečnost způsobem typu:

"accept all certificates"

pokud tomu lze zabránit.

============================================================
10. INTEGRITA FIRMWARE
============================================================

Implementuj ověření integrity firmware.

Preferuj SHA-256.

Podporuj:

firmware.bin
firmware.bin.sha256

Pokud Release obsahuje checksum:

1. zjisti checksum,
2. stáhni firmware,
3. vypočítej SHA-256,
4. porovnej checksum,
5. pokračuj pouze při shodě.

Při nesouladu:

OTA zrušit.

Nikdy nespouštěj firmware s neověřenou integritou.

============================================================
11. OTA DOWNLOAD
============================================================

Download musí být prováděn bezpečně.

Před downloadem ověř:

- Release,
- verzi,
- target,
- asset,
- velikost,
- OTA partition,
- případně checksum metadata.

Během downloadu:

- používej timeouty,
- správně zpracuj přerušení,
- nevytvářej zbytečnou kopii celého firmware v RAM,
- streamuj firmware přímo do OTA partition, pokud to framework podporuje,
- pravidelně udržuj síť/watchdog podle architektury projektu.

Pokud download selže:

OTA se ukončí.

Současný firmware musí zůstat funkční.

============================================================
12. OCHRANA PŘED VÝPADEMKEM NAPÁJENÍ
============================================================

Použij A/B OTA.

Během OTA musí:

aktuální firmware
    ↓
zůstat v původní OTA partition

nový firmware
    ↓
být zapisován do druhé OTA partition

Nikdy nemaž současný firmware před úspěšným dokončením nového.

Pokud během OTA vypadne napájení:

po restartu musí ESP stále být schopné nabootovat funkční firmware.

Použij standardní boot/OTA mechanismus konkrétního frameworku.

============================================================
13. ROLLBACK
============================================================

Implementuj rollback, pokud ho konkrétní hardware/framework podporuje.

Po OTA restartu nový firmware nesmí být okamžitě považován za definitivně funkční.

Nový firmware musí po startu ověřit, že:

- základní inicializace proběhla,
- zařízení není v boot loop,
- kritické periferie jsou inicializované,
- firmware běží stabilně,
- síťová část se inicializuje, pokud je pro projekt kritická.

Teprve potom označ nový firmware jako validovaný.

Pokud nový firmware opakovaně selhává:

rollback na předchozí firmware.

============================================================
14. OCHRANA PŘED NEKONEČNÝM OTA LOOP
============================================================

Toto je POVINNÉ.

Příklad:

ESP = 1.0.0
GitHub = 1.1.0

OTA → 1.1.0

1.1.0 selže.

ESP rollbackne na 1.0.0.

ESP NESMÍ okamžitě znovu stáhnout 1.1.0 a opakovat stejný neúspěšný update donekonečna.

Ulož informaci o posledním neúspěšném firmware/release.

Pokud byl konkrétní release již označen jako neúspěšný, další automatický pokus o stejnou verzi neprováděj, dokud:
- není vydána novější verze,
- nebo není chyba ručně resetována vhodným způsobem.

Persistentní stav ukládej bezpečně do vhodného persistentního úložiště.

============================================================
15. BEZPEČNÝ STAV PŘED OTA
============================================================

Před zahájením OTA připrav zařízení do bezpečného stavu.

Analyzuj, co projekt ovládá.

Pokud zařízení používá:
- relé,
- motory,
- topení,
- ventilátory,
- serva,
- MOSFETy,
- jiné výstupy,

zajisti, aby OTA nebyla spuštěna v nebezpečném okamžiku.

Před OTA podle architektury projektu:

- ulož důležitá data,
- dokonči důležité zápisy,
- nastav kritické výstupy do bezpečného stavu,
- případně OTA odlož, pokud právě probíhá kritická operace.

OTA nesmí například vypnout nebo změnit kritický výstup uprostřed nebezpečné operace bez kontroly.

Nevymýšlej univerzální "safe state" naslepo. Přizpůsob ho skutečné funkci projektu.

============================================================
16. WATCHDOG
============================================================

OTA nesmí způsobit watchdog reset kvůli dlouhému downloadu.

Respektuj existující watchdog.

Pokud OTA probíhá dlouho:

- správně obsluhuj watchdog podle frameworku,
- nepoužívej nekonečné blokování,
- používej timeouty.

Po chybě musí OTA skončit kontrolovaně.

============================================================
17. OTA INTERVAL
============================================================

OTA kontrola nesmí běžet v každém loop().

Použij konfigurovatelný interval.

Výchozí:

30 minut.

Po úspěšném připojení k internetu může proběhnout první kontrola.

OTA kontrola nesmí blokovat hlavní funkci zařízení více, než je nutné.

============================================================
18. OTA NENÍ KRITICKÁ ZÁVISLOST
============================================================

Zařízení musí fungovat i bez internetu.

GitHub nesmí být nutný pro:
- boot,
- normální provoz,
- hlavní funkce zařízení.

OTA je pouze doplňková funkce.

Pokud GitHub nefunguje:

zařízení normálně pokračuje ve stávajícím firmware.

============================================================
19. PERSISTENTNÍ DATA
============================================================

OTA nesmí mazat:

- NVS,
- EEPROM data,
- LittleFS,
- SPIFFS,
- konfiguraci,
- kalibrace,
- uživatelská nastavení,

pokud to není výslovně nutné.

Pokud nová verze firmware vyžaduje migraci dat:

implementuj bezpečný migration mechanismus.

Nikdy automaticky nemaž persistentní data pouze kvůli OTA.

============================================================
20. BOOT VALIDACE
============================================================

Pokud framework podporuje boot validation / rollback mechanismus:

využij ho.

Nový firmware musí být označen jako validovaný až po úspěšném startu.

Pokud projekt obsahuje vlastní inicializační sekvenci, integruj validaci do vhodného místa.

Boot validation nesmí způsobit falešný rollback při pomalejším startu.

============================================================
21. LOGOVÁNÍ
============================================================

Použij existující logging systém projektu.

Přidej přehledné OTA logy.

Například:

[OTA] Checking for updates...
[OTA] Current version: 1.0.0
[OTA] Latest version: 1.1.0
[OTA] Target: ESP32-XXX
[OTA] Firmware size: XXXXX bytes
[OTA] OTA partition size: XXXXX bytes
[OTA] Size check: OK
[OTA] Downloading firmware...
[OTA] Download progress: XX%
[OTA] Verifying SHA-256...
[OTA] Verification successful
[OTA] Starting OTA...
[OTA] OTA completed
[OTA] Rebooting...

Chyby:

[OTA] No update available
[OTA] GitHub unavailable
[OTA] Release not found
[OTA] Firmware asset not found
[OTA] Firmware too large
[OTA] Incompatible firmware
[OTA] Download failed
[OTA] Checksum mismatch
[OTA] OTA write failed
[OTA] Update cancelled
[OTA] Firmware previously failed
[OTA] Rollback detected

============================================================
22. KONFIGURACE
============================================================

Vytvoř jedno přehledné místo pro konfiguraci OTA.

Minimálně:

GITHUB_OWNER
GITHUB_REPOSITORY
FIRMWARE_ASSET
OTA_CHECK_INTERVAL

Podle potřeby:

OTA_TIMEOUT
OTA_CONNECT_TIMEOUT
OTA_DOWNLOAD_TIMEOUT
OTA_ENABLED

Nepřidávej žádné secrets.

============================================================
23. RELEASE FORMÁT
============================================================

Dokumentuj přesný formát Release.

Například:

Release:
v1.2.0

Assets:

firmware.bin
firmware.bin.sha256

Pokud je použit target metadata soubor, zdokumentuj ho.

Například případně:

firmware.json

s informacemi jako:

version
target
firmware_size
sha256

Pokud vytvoření metadata souboru výrazně zvýší robustnost systému, implementuj ho.

ESP musí metadata před samotným OTA ověřit.

============================================================
24. RUČNÍ RELEASE WORKFLOW
============================================================

README musí popsat tento postup:

1. Upravit firmware.
2. Změnit FIRMWARE_VERSION.
3. Zkontrolovat build.
4. Ručně zkompilovat firmware.
5. Vytvořit firmware.bin.
6. Vytvořit SHA-256 checksum, pokud je implementován.
7. Vytvořit GitHub Release.

Například:

v1.2.0

8. Přidat:

firmware.bin

a případně:

firmware.bin.sha256

9. Release publikovat.
10. ESP při další kontrole zjistí novou verzi.
11. Provede všechny bezpečnostní kontroly.
12. Provede OTA.

============================================================
25. ŽÁDNÉ GITHUB ACTIONS
============================================================

NEVYTVÁŘEJ:

.github/workflows/
GitHub Actions
automatický build
automatickou kompilaci
automatický Release
automatický upload
automatické tagování.

Všechno provádím ručně.

============================================================
26. README
============================================================

Vytvoř nebo aktualizuj README.

README musí vysvětlovat:

- jak OTA funguje,
- jaká je velikost flash,
- jak je flash rozdělena,
- velikost OTA_0,
- velikost OTA_1,
- maximální velikost firmware,
- kde je OTA konfigurace,
- jak vytvořit firmware.bin,
- jak vytvořit checksum,
- jak vytvořit Release,
- jak Release pojmenovat,
- jak přidat asset,
- jak ESP kontroluje novou verzi,
- jak se kontroluje velikost,
- jak se kontroluje SHA-256,
- jak funguje rollback,
- co se stane při výpadku napájení,
- co se stane při výpadku internetu,
- co se stane při nekompatibilním firmware,
- co se stane při příliš velkém firmware,
- jak se zabrání OTA loopu.

============================================================
27. TESTOVÁNÍ
============================================================

Po implementaci proveď co nejvíce testů bez hardware.

Ověř:

- projekt se kompiluje,
- OTA kód se kompiluje,
- správný board,
- správný partition layout,
- dvě OTA partitions,
- velikost OTA partition,
- kontrolu velikosti firmware,
- verzování,
- porovnávání verzí,
- GitHub API URL,
- HTTPS,
- checksum,
- rollback,
- persistentní stav failed release,
- watchdog,
- timeouty,
- kompatibilitu s existujícím systémem.

Pokud je možné vytvořit unit testy pro version comparison nebo metadata parsing, vytvoř je.

============================================================
28. NEGATIVNÍ TEST SCÉNÁŘE
============================================================

Ověř nebo alespoň analyzuj následující scénáře:

1. Není internet.
2. Není Wi-Fi.
3. GitHub neodpovídá.
4. GitHub API vrátí chybu.
5. Release neexistuje.
6. Release neobsahuje firmware.bin.
7. Firmware je příliš velký.
8. Firmware má špatný checksum.
9. Firmware je pro jiný target.
10. Download se přeruší.
11. ESP se restartuje během OTA.
12. Napájení vypadne během OTA.
13. Nový firmware po bootu selže.
14. Nový firmware způsobuje boot loop.
15. Rollback proběhne.
16. Po rollbacku se ESP nesmí znovu pokoušet instalovat stejnou vadnou verzi.
17. Není dostupná persistentní konfigurace.
18. Firmware je stejná verze jako aktuální.
19. GitHub má starší Release než aktuální firmware.
20. Firmware je větší než dostupná OTA partition.
21. Content-Length není dostupný nebo nesedí.
22. Download je pomalý.
23. OTA probíhá déle než očekávaný watchdog timeout.

Pro každý případ zajisti bezpečné chování.

============================================================
29. ZÁSADA PRO ZMĚNY PROJEKTU
============================================================

Neměň zbytečně existující kód.

Preferuj:
- malé izolované OTA moduly,
- jasné rozhraní,
- minimální zásahy do hlavního programu.

Pokud například vytvoříš:

OTAManager

nebo podobný modul, integruj ho čistě.

Hlavní program by měl OTA pouze inicializovat a případně volat kontrolu podle architektury projektu.

============================================================
30. ZÁVĚREČNÝ REPORT
============================================================

Po dokončení vypiš:

A) Přesný typ ESP/board.

B) Použitý framework.

C) Velikost flash.

D) Partition layout.

E) Velikost OTA_0.

F) Velikost OTA_1.

G) Maximální velikost firmware pro OTA.

H) Velikost filesystemu.

I) Jaké soubory byly vytvořeny.

J) Jaké soubory byly upraveny.

K) Jak OTA funguje.

L) Jak je chráněn současný firmware.

M) Jak funguje kontrola velikosti.

N) Jak funguje checksum.

O) Jak funguje kompatibilita hardware.

P) Jak funguje rollback.

Q) Jak je zabráněno nekonečnému OTA loopu.

R) Jak je řešen výpadek napájení.

S) Jak je řešen watchdog.

T) Jak je řešen bezpečný stav zařízení před OTA.

U) Jak vytvořím firmware.bin.

V) Jak vytvořím GitHub Release.

W) Jaké assety musí Release obsahovat.

X) Jaké testy byly provedeny.

Y) Co nelze ověřit bez fyzického zařízení.

Z) Jaké případné problémy nebo omezení ještě existují.

============================================================
31. ABSOLUTNÍ PRAVIDLA
============================================================

NIKDY:
- nepřepisuj běžící firmware před bezpečným dokončením OTA,
- nespouštěj firmware, který se nevejde do OTA partition,
- nespouštěj firmware s neplatným checksumem,
- nespouštěj firmware pro jiný target,
- nemaž persistentní data kvůli OTA,
- nezpůsob, aby GitHub byl nutný pro normální provoz,
- nedovol nekonečný OTA loop,
- neprováděj automatický GitHub Release,
- nevytvářej GitHub Actions,
- nedělej git commit,
- nedělej git push.

Pokud si nejsi jistý nějakou částí architektury, nejdříve ji analyzuj a použij řešení odpovídající skutečnému projektu.

Nejdříve ANALYZUJ.
Potom IMPLEMENTUJ.
Nakonec OTESTUJ a vytvoř závěrečný report.