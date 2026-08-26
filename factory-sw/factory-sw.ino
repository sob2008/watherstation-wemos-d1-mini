// ============================================================
// TOVARNI SOFTWARE METEOSTANICE
// ============================================================
// Nahravat POUZE pres factory-sw/!flash (viz README v tomto adresari) na
// nove/vracene kusy pred expedici zakaznikovi. Ucel: provede zakaznika
// pripojenim k jeho domaci WiFi a hned poté si samo stahne a nainstaluje
// nejnovejsi ostry firmware (wather-station-2dis) z GitHub Releases -
// pouziva presne stejny OTA klient (OtaManager/OtaState/...) jako ostry
// firmware, jen zkopirovany do teto slozky (Arduino kompiluje kazdy sketch
// zvlast, cross-folder include neni mozny).
//
// Po uspesne instalaci se zarizeni samo restartuje a dal uz bezi jako
// normalni meteostanice - tento sketch se tim prepise a jiz nikdy nebezi
// znovu (dokud by nekdo rucne nenahral factory-sw pres USB podruhe, napr.
// pri vraceni/reklamaci).

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <WiFiManager.h>

#include "OtaConfig.h"
#include "OtaState.h"
#include "OtaManager.h"
#include "LocationConfig.h"
#include "Lang.h"

// Piny - stejne jako ostry firmware (wather-station-2dis), tovarni SW bezi
// na identickem hardwaru. Kazdy displej ma vlastni CS a RESET, viz
// wather-station-2dis/DOKUMENTACE.txt.
#define CS1_PIN 4    // D2 (GPIO4)  - Displej 1 CS
#define CS2_PIN 12   // D6 (GPIO12) - Displej 2 CS
#define RST1_PIN 5   // D1 (GPIO5)  - Displej 1 RESET
#define RST2_PIN 2   // D4 (GPIO2)  - Displej 2 RESET
#define DC_PIN 0     // D3 (GPIO0)  - Sdileny DC
#define CLK_PIN 14   // D5 (GPIO14) - Sdileny CLK
#define DATA_PIN 13  // D7 (GPIO13) - Sdileny DATA

U8G2_SH1106_128X64_NONAME_F_4W_SW_SPI disp1(U8G2_R0, CLK_PIN, DATA_PIN, CS1_PIN, DC_PIN, RST1_PIN);
U8G2_SH1106_128X64_NONAME_F_4W_SW_SPI disp2(U8G2_R0, CLK_PIN, DATA_PIN, CS2_PIN, DC_PIN, RST2_PIN);

// Hlavni (zakaznicky) status na displeji 1 - nadpis + az dva radky textu.
void showStatus(const char* title, const char* line1, const char* line2 = "") {
  disp1.clearBuffer();
  disp1.drawRFrame(0, 0, 128, 64, 4);
  disp1.setFont(u8g2_font_7x14B_tf);
  disp1.drawStr(10, 16, title);
  disp1.drawHLine(6, 20, 116);
  disp1.setFont(u8g2_font_6x10_tf);
  disp1.drawStr(10, 36, line1);
  if (line2 && line2[0]) {
    disp1.drawStr(10, 50, line2);
  }
  disp1.sendBuffer();
}

// Sekundarni displej - jednoduse doplnkove info, at nezustava prazdny.
void showSecondary(const char* line1, const char* line2) {
  disp2.clearBuffer();
  disp2.drawRFrame(0, 0, 128, 64, 4);
  disp2.setFont(u8g2_font_6x10_tf);
  disp2.drawStr(15, 30, line1);
  disp2.drawStr(15, 45, line2);
  disp2.sendBuffer();
}

// Volano z OtaManager behem aktivniho stahovani/flashovani.
void otaStatusCallback(const String& line1, const String& line2) {
  showStatus(Lang::t(LangId::FactoryInstallingTitle), line1.c_str(), line2.c_str());
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("================================================");
  Serial.println("   TOVARNI SOFTWARE METEOSTANICE");
  Serial.println("================================================");
  Serial.println("Ucel: pripoji zarizeni k WiFi zakaznika a rovnou");
  Serial.println("nainstaluje aktualni ostry firmware z GitHub Releases.");
  Serial.print("Cilova platforma (FIRMWARE_TARGET): ");
  Serial.println(FIRMWARE_TARGET);
  Serial.println();

  disp1.begin();
  delay(10);
  disp2.begin();
  delay(10);

  showStatus("METEOSTANICE", "Tovarni priprava", "zarizeni...");
  showSecondary("Vitejte!", "Chvilku strpeni");
  delay(2000);

  // LittleFS + perzistentni OTA stav, lokalita a jazyk. Na zcela novem/prave
  // smazanem zarizeni (flash.py/flash.sh delaji plny erase_flash pred
  // zapisem) je tohle vzdy cisty start - viz README v tomto adresari.
  OtaState::begin();
  OtaManager::setStatusCallback(otaStatusCallback);
  OtaManager::begin();
  LocationConfig::begin();
  Lang::begin();

  // --- Instrukce pro zakaznika ---
  Serial.println("KROK 1: Pripojte se telefonem nebo pocitacem na WiFi sit:");
  Serial.println("        MeteoStation_AP");
  Serial.println("        Pak v prohlizeci otevrete: 192.168.4.1");
  Serial.println("        a vyberte vasi domaci WiFi sit + zadejte obec/mesto.");
  Serial.println();

  showStatus(Lang::t(LangId::FactoryStep1), Lang::t(LangId::FactoryConnectToWifi), "MeteoStation_AP");
  showSecondary(Lang::t(LangId::FactoryThenEnter), Lang::t(LangId::FactoryWifiAndCity));

  // Stejny vzor jako wather-station-2dis.ino: pri vyprseni portalu radeji
  // cely ESP restartuje, nez aby se autoConnect() volalo znovu na tomtez
  // WiFiManager objektu - to je overene chovani, opakovane volani na jiz
  // pouzitem WiFiManager instance overene neni.
  WiFiManager wm;

  // Vlastni pole v portalu pro obec/mesto zakaznika - viz LocationConfig.h.
  String cityBefore = LocationConfig::name();
  WiFiManagerParameter cityParam(
      "city", "Vase obec/mesto", cityBefore.c_str(), 40,
      "placeholder='napr. Rojetin, okres Sumperk'");
  wm.addParameter(&cityParam);

  // Jazyk zobrazeni ("cs"/"en") - viz Lang.h.
  String langBefore = String(Lang::code());
  WiFiManagerParameter langParam(
      "lang", "Jazyk / Language (cs/en)", langBefore.c_str(), 2,
      "placeholder='cs'");
  wm.addParameter(&langParam);

  wm.setConfigPortalTimeout(180);
  if (!wm.autoConnect("MeteoStation_AP")) {
    Serial.println("Konfiguracni portal WiFi vyprsel (180s), restartuji...");
    showStatus(Lang::t(LangId::FactoryStep1), Lang::t(LangId::FactoryRetrying), "");
    delay(2000);
    ESP.restart();
  }

  // Zpracovat pripadnou zmenu jazyka JESTE PRED zbytkem obrazovek, aby uz
  // pouzily spravny jazyk.
  Lang::setLanguage(String(langParam.getValue()));

  Serial.print("WiFi pripojeno. IP adresa: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  showStatus(Lang::t(LangId::FactoryWifiConnected), WiFi.localIP().toString().c_str(), "");
  showSecondary(Lang::t(LangId::FactoryConnectedExcl), "");
  delay(1500);

  // Zpracovat zadanou lokalitu.
  String cityAfter = String(cityParam.getValue());
  cityAfter.trim();
  if (cityAfter.length() > 0 && cityAfter != cityBefore) {
    Serial.print("Zadana lokalita: ");
    Serial.println(cityAfter);

    showStatus(Lang::t(LangId::FactoryLocation), Lang::t(LangId::FactorySearching), cityAfter.c_str());

    if (!LocationConfig::geocodeAndSave(cityAfter)) {
      Serial.println("Lokalitu se nepodarilo najit, pouziva se vychozi - zakaznik ji");
      Serial.println("muze pozdeji zmenit dvojitym RESETem na ostrem firmware.");
      showStatus(Lang::t(LangId::FactoryLocation), Lang::t(LangId::FactoryNotFound), Lang::t(LangId::FactoryUsingDefault));
      delay(1500);
    } else {
      showStatus(Lang::t(LangId::FactoryLocation), Lang::t(LangId::FactoryFound), LocationConfig::name().c_str());
      delay(1000);
    }
  }

  Serial.println("KROK 2: Stahuji a instaluji aktualni verzi softwaru...");
  Serial.println("        (podrobny prubeh viz [OTA] hlasky nize)");
  Serial.println();

  showStatus(Lang::t(LangId::FactoryStep2), Lang::t(LangId::FactoryInstalling), Lang::t(LangId::FactorySoftware));
  showSecondary(Lang::t(LangId::FactoryPleaseWaitCaps), Lang::t(LangId::FactoryDoNotUnplug));
}

void loop() {
  // OtaManager::handle() pri uspesne instalaci sam zavola ESP.restart() -
  // od tohoto okamziku uz dal bezi ostry firmware a tento sketch se
  // nevraci. Pokud se sem loop() vrati, aktualizace se (zatim) nepovedla
  // (napr. GitHub docasne nedostupny, nebo zadny kompatibilni Release
  // jeste neexistuje) - OTA_CHECK_INTERVAL_MS v factory-sw/OtaConfig.h je
  // umyslne kratky (20s), takze se to zkusi znovu za chvíli. Presny duvod
  // vidite v Serial Monitoru (hlasky "[OTA] ...").
  OtaManager::handle();
  delay(200);
}
