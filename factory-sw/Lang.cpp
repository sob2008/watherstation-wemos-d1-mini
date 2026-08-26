#include "Lang.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <string.h>

namespace Lang {

namespace {

const char* kDir = "/config";
const char* kPath = "/config/language.json";
const char* kTmpPath = "/config/language.json.tmp";

bool g_english = false;

// Poradi MUSI presne odpovidat enum LangId v Lang.h - kontrolovano
// staticky nize (kCzech/kEnglish musi mit presne kLangIdCount polozek).
const char* const kCzech[] = {
  "Jasno", "Skoro jasno", "Polojasno", "Zatazeno", "Mlha",
  "Slabe mrholeni", "Mrholeni", "Silne mrholeni", "Mraz. mrholeni",
  "Slaby dest", "Dest", "Silny dest", "Mraz. dest",
  "Slabe snezeni", "Snezeni", "Silne snezeni", "Sneh. zrna",
  "Slaba preh.", "Prehanka", "Silna preh.", "Sneh. preh.",
  "Boure", "Boure s krup.",

  "Jasno", "Polojasno", "Zatazeno", "Mlha", "Mrholeni", "Mrz.mrh.",
  "Dest", "Mrz.dest", "Snezeni", "Sn.zrna", "Prehanky", "Sn.preh.",
  "Boure", "---",

  "S", "SV", "V", "JV", "J", "JZ", "Z", "SZ",

  "Nacitam data...", "Cekam na data...", "Nacitam...",
  "ZITRA", "POZITRI", "Vlhkost:", "Vitr:", "Srazky:",

  "METEO", "STANICE", "Sekundarni", "displej", "verze",
  "Pripojuji WiFi...", "Cekam na WiFi", "WiFi CHYBA!",
  "Hledam lokalitu:", "PRIPOJENO!", "WiFi OK!",

  "Vitejte!", "Chvilku strpeni", "KROK 1/2", "Pripojte se na WiFi:",
  "Pak zadejte", "WiFi + obec", "Zkousim znovu...", "WIFI PRIPOJENO",
  "Pripojeno!", "LOKALITA", "Hledam:", "Nenalezena,", "pouzivam vychozi",
  "Nalezena:", "KROK 2/2", "Instaluji aktualni", "software...",
  "Prosim cekejte", "Nevypinejte napajeni", "INSTALUJI SOFTWARE",
};

const char* const kEnglish[] = {
  "Clear", "Mostly clear", "Partly cloudy", "Cloudy", "Fog",
  "Light drizzle", "Drizzle", "Heavy drizzle", "Freezing drizzle",
  "Light rain", "Rain", "Heavy rain", "Freezing rain",
  "Light snow", "Snow", "Heavy snow", "Snow grains",
  "Light showers", "Showers", "Heavy showers", "Snow showers",
  "Thunderstorm", "Storm w/ hail",

  "Clear", "P.Cloudy", "Cloudy", "Fog", "Drizzle", "Fz.driz.",
  "Rain", "Fz.rain", "Snow", "Sn.grain", "Showers", "Sn.show.",
  "Storm", "---",

  "N", "NE", "E", "SE", "S", "SW", "W", "NW",

  "Loading data...", "Waiting for data...", "Loading...",
  "TOMORROW", "DAY AFTER", "Humidity:", "Wind:", "Precip:",

  "WEATHER", "STATION", "Secondary", "display", "version",
  "Connecting WiFi...", "Waiting for WiFi", "WiFi ERROR!",
  "Finding location:", "CONNECTED!", "WiFi OK!",

  "Welcome!", "Please wait", "STEP 1/2", "Connect to WiFi:",
  "Then enter", "WiFi + city", "Retrying...", "WIFI CONNECTED",
  "Connected!", "LOCATION", "Searching:", "Not found,", "using default",
  "Found:", "STEP 2/2", "Installing latest", "software...",
  "Please wait", "Do not unplug power", "INSTALLING SOFTWARE",
};

static_assert(sizeof(kCzech) / sizeof(kCzech[0]) == static_cast<size_t>(LangId::kLangIdCount),
              "kCzech musi mit presne tolik polozek jako LangId");
static_assert(sizeof(kEnglish) / sizeof(kEnglish[0]) == static_cast<size_t>(LangId::kLangIdCount),
              "kEnglish musi mit presne tolik polozek jako LangId");

bool writeToDisk() {
  JsonDocument doc;
  doc["lang"] = g_english ? "en" : "cs";

  File f = LittleFS.open(kTmpPath, "w");
  if (!f) {
    Serial.println("[LANG] ERROR: cannot open language tmp file for write");
    return false;
  }
  size_t written = serializeJson(doc, f);
  f.close();
  if (written == 0) {
    Serial.println("[LANG] ERROR: language serialization wrote 0 bytes");
    LittleFS.remove(kTmpPath);
    return false;
  }
  if (!LittleFS.rename(kTmpPath, kPath)) {
    Serial.println("[LANG] ERROR: language rename failed");
    return false;
  }
  return true;
}

void readFromDisk() {
  if (!LittleFS.exists(kPath)) {
    return; // zadny ulozeny jazyk jeste neexistuje - vychozi cestina je spravne
  }
  File f = LittleFS.open(kPath, "r");
  if (!f) {
    Serial.println("[LANG] WARNING: language.json exists but cannot be opened, using default");
    return;
  }
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    Serial.print("[LANG] WARNING: language.json corrupt (");
    Serial.print(err.c_str());
    Serial.println("), using default");
    return;
  }
  const char* code = doc["lang"] | "cs";
  g_english = (strcmp(code, "en") == 0);
}

} // namespace

void begin() {
  if (!LittleFS.begin()) {
    Serial.println("[LANG] ERROR: LittleFS unavailable, language persistence disabled");
    return;
  }
  if (!LittleFS.exists(kDir)) {
    LittleFS.mkdir(kDir);
  }
  readFromDisk();
}

void setLanguage(const String& codeIn) {
  String c = codeIn;
  c.trim();
  c.toLowerCase();
  bool newEnglish;
  if (c == "en") {
    newEnglish = true;
  } else if (c == "cs") {
    newEnglish = false;
  } else {
    return; // neplatna hodnota - jazyk se nemeni
  }
  if (newEnglish == g_english) {
    return; // beze zmeny, netreba zapisovat na flash
  }
  g_english = newEnglish;
  writeToDisk();
}

bool isEnglish() { return g_english; }
const char* code() { return g_english ? "en" : "cs"; }

const char* t(LangId id) {
  size_t idx = static_cast<size_t>(id);
  if (idx >= static_cast<size_t>(LangId::kLangIdCount)) {
    return "";
  }
  return g_english ? kEnglish[idx] : kCzech[idx];
}

} // namespace Lang
