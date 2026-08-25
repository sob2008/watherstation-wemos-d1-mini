# Návrat do této konverzace (Claude Code)

**Otevřít terminál ve složce:**
```
C:\Users\Lenovo\Desktop\!PROJEKTY\!lunux_arduino\wather-station
```

**A napsat:**
```
claude --resume watherstation
```

---

Tahle konverzace má session ID `88549ac3-dcf4-4e87-b19e-d4038663692b` a je pojmenovaná
`watherstation` (přes `/rename watherstation` přímo v konverzaci).

## Další možnosti

Pokud by `--resume watherstation` z nejakého důvodu nenašel přesnou konverzaci (např. časem
vznikne víc konverzací se stejným/podobným názvem), otevře se místo toho interaktivní seznam
filtrovaný podle "watherstation" - tuhle konverzaci pak vyberte ručně podle času.

Spolehlivá záloha přes session ID:
```
claude --resume 88549ac3-dcf4-4e87-b19e-d4038663692b
```

`claude --continue` (nebo `claude -c`) obnoví automaticky **poslední** konverzaci v tomto adresáři -
funguje jen dokud nezaložíte novější.

## Co se v této konverzaci udělalo (shrnutí)

1. **Inicializace projektu** - vytvořen `CLAUDE.md` (`/init`).
2. **OTA systém** (podle `wather-station-2dis/idea.md`) - firmware si sám kontroluje a instaluje
   aktualizace z GitHub Releases: `OtaConfig.h`, `OtaVersion.*`, `Sha256.*`, `OtaState.*`,
   `OtaManager.*` v `wather-station-2dis/`. Rollback na `last_good.bin`, ochrana proti nekonečné
   smyčce, povinná SHA-256 kontrola integrity (TLS zůstává `setInsecure()` - zdůvodněno v
   `README.md`).
3. **Automatizace release** - `.github/workflows/release.yml` (build + GitHub Release, spouští se
   jen ručním pushnutím tagu `vX.Y.Z`) a `scripts/release.ps1` (bump verze + commit + tag + push
   jedním příkazem).
4. **`factory-sw/`** - samostatný tovární firmware pro nové/vrácené kusy: provede zákazníka WiFi
   setupem a hned nainstaluje aktuální Release. Sdílí OTA klienta se skutečným firmware
   (zkopírovaný, ne linkovaný - Arduino kompiluje sketch složky zvlášť). `!flash/flash.py` a
   `!flash/flash.sh` po flashnutí automaticky otevřou sériový monitor.
5. Několik kol revize kódu (ověřeno reálnou kompilací přes `arduino-cli` + host-side testy v
   `tests/`) - opraveny mj. race condition v `OtaState.cpp` (remove-před-rename), riskantní
   WiFiManager retry vzor ve `factory-sw`, a chybějící ochrana proti záměně `.bin` souborů ve
   sdílené `!flash/bin/` složce.

Vše je commitnuté a napushnuté na `origin/main` (poslední commit `ce021ba`).
