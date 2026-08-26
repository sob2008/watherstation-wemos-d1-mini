#include "LocationConfig.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ctype.h>

namespace LocationConfig {

namespace {

// Vychozi hodnoty, dokud zakaznik nezada vlastni obec (nebo pokud
// geokodovani selze a zadna predchozi lokalita jeste neni ulozena).
const char* kDefaultName = "ROJETIN";
const float kDefaultLat = 49.36f;
const float kDefaultLon = 16.26f;

const char* kDir = "/config";
const char* kPath = "/config/location.json";
const char* kTmpPath = "/config/location.json.tmp";

String g_name = kDefaultName;
float g_lat = kDefaultLat;
float g_lon = kDefaultLon;
bool g_configured = false;

bool writeToDisk() {
  JsonDocument doc;
  doc["name"] = g_name;
  doc["lat"] = g_lat;
  doc["lon"] = g_lon;

  File f = LittleFS.open(kTmpPath, "w");
  if (!f) {
    Serial.println("[LOC] ERROR: cannot open location tmp file for write");
    return false;
  }
  size_t written = serializeJson(doc, f);
  f.close();
  if (written == 0) {
    Serial.println("[LOC] ERROR: location serialization wrote 0 bytes");
    LittleFS.remove(kTmpPath);
    return false;
  }

  // rename() cilovy soubor pokud existuje prepise atomicky sam (viz
  // OtaState.cpp - stejny duvod, proc se sem nedava LittleFS.remove(kPath)
  // pred timto volanim).
  if (!LittleFS.rename(kTmpPath, kPath)) {
    Serial.println("[LOC] ERROR: location rename failed");
    return false;
  }
  return true;
}

void readFromDisk() {
  if (!LittleFS.exists(kPath)) {
    return; // zadna ulozena lokalita jeste neexistuje - vychozi hodnoty jsou spravne
  }
  File f = LittleFS.open(kPath, "r");
  if (!f) {
    Serial.println("[LOC] WARNING: location.json exists but cannot be opened, using defaults");
    return;
  }
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    Serial.print("[LOC] WARNING: location.json corrupt (");
    Serial.print(err.c_str());
    Serial.println("), using defaults");
    return;
  }

  const char* n = doc["name"] | "";
  if (strlen(n) == 0) {
    return;
  }
  g_name = String(n);
  g_lat = doc["lat"] | g_lat;
  g_lon = doc["lon"] | g_lon;
  g_configured = true;
}

// Mapovani UTF-8 bajtovych dvojic ceskych pismen s diakritikou na "holy"
// ASCII zaklad - pouziva se pro zobrazeni na displeji (fonty tam hacky/carky
// neumi, viz DOKUMENTACE.txt), zatimco puvodni nazev se zadanou diakritikou
// jde jako geokodovaci dotaz (viz geocodeAndSave nize - presnejsi shoda).
struct DiacriticMap {
  uint8_t b1;
  uint8_t b2;
  char ascii;
};

const DiacriticMap kDiacritics[] = {
  {0xC3, 0x81, 'A'}, {0xC3, 0xA1, 'a'},  // A a
  {0xC4, 0x8C, 'C'}, {0xC4, 0x8D, 'c'},  // C c (hacek)
  {0xC4, 0x8E, 'D'}, {0xC4, 0x8F, 'd'},  // D d (hacek)
  {0xC3, 0x89, 'E'}, {0xC3, 0xA9, 'e'},  // E e
  {0xC4, 0x9A, 'E'}, {0xC4, 0x9B, 'e'},  // E e (hacek)
  {0xC3, 0x8D, 'I'}, {0xC3, 0xAD, 'i'},  // I i
  {0xC5, 0x87, 'N'}, {0xC5, 0x88, 'n'},  // N n (hacek)
  {0xC3, 0x93, 'O'}, {0xC3, 0xB3, 'o'},  // O o
  {0xC5, 0x98, 'R'}, {0xC5, 0x99, 'r'},  // R r (hacek)
  {0xC5, 0xA0, 'S'}, {0xC5, 0xA1, 's'},  // S s (hacek)
  {0xC5, 0xA4, 'T'}, {0xC5, 0xA5, 't'},  // T t (hacek)
  {0xC3, 0x9A, 'U'}, {0xC3, 0xBA, 'u'},  // U u
  {0xC5, 0xAE, 'U'}, {0xC5, 0xAF, 'u'},  // U u (krouzek)
  {0xC3, 0x9D, 'Y'}, {0xC3, 0xBD, 'y'},  // Y y
  {0xC5, 0xBD, 'Z'}, {0xC5, 0xBE, 'z'},  // Z z (hacek)
};

// Prevede UTF-8 retezec na ASCII: rozpoznana ceska pismena s diakritikou se
// nahradi holym zakladem, ostatni vicebajtove UTF-8 znaky (neceka
// diakritika) se zahodi (radeji nic nez smetí na displeji), cisty ASCII
// projde beze zmeny.
String stripDiacritics(const String& utf8) {
  String out;
  out.reserve(utf8.length());
  size_t i = 0;
  size_t len = utf8.length();
  while (i < len) {
    uint8_t b1 = static_cast<uint8_t>(utf8[i]);
    if (b1 < 0x80) {
      out += static_cast<char>(b1);
      i += 1;
    } else if ((b1 & 0xE0) == 0xC0 && i + 1 < len) {
      uint8_t b2 = static_cast<uint8_t>(utf8[i + 1]);
      for (size_t k = 0; k < sizeof(kDiacritics) / sizeof(kDiacritics[0]); k++) {
        if (kDiacritics[k].b1 == b1 && kDiacritics[k].b2 == b2) {
          out += kDiacritics[k].ascii;
          break;
        }
      }
      i += 2;
    } else if ((b1 & 0xF0) == 0xE0 && i + 2 < len) {
      i += 3; // 3bajtovy UTF-8 znak mimo ceskou diakritiku - zahodit
    } else if ((b1 & 0xF8) == 0xF0 && i + 3 < len) {
      i += 4; // 4bajtovy UTF-8 znak - zahodit
    } else {
      i += 1; // neplatny/osamoceny bajt - preskocit
    }
  }
  return out;
}

// Procentualni zakodovani pro pouziti jako hodnota query parametru v URL
// (jmeno obce muze obsahovat mezery, carky a ceske diakritiky - vse jsou
// v UTF-8 vicebajtove znaky, koduji se bajt po bajtu).
String urlEncode(const String& value) {
  String encoded = "";
  char buf[4];
  for (size_t i = 0; i < value.length(); i++) {
    uint8_t c = static_cast<uint8_t>(value[i]);
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += static_cast<char>(c);
    } else if (c == ' ') {
      encoded += '+';
    } else {
      snprintf(buf, sizeof(buf), "%%%02X", c);
      encoded += buf;
    }
  }
  return encoded;
}

} // namespace

void begin() {
  if (!LittleFS.begin()) {
    Serial.println("[LOC] ERROR: LittleFS unavailable, location persistence disabled");
    return;
  }
  if (!LittleFS.exists(kDir)) {
    LittleFS.mkdir(kDir);
  }
  readFromDisk();
}

bool isConfigured() { return g_configured; }
const String& name() { return g_name; }
float lat() { return g_lat; }
float lon() { return g_lon; }

bool geocodeAndSave(const String& query) {
  String trimmed = query;
  trimmed.trim();
  if (trimmed.length() == 0) {
    return false;
  }

  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(10000);
  HTTPClient http;

  String url = "https://geocoding-api.open-meteo.com/v1/search?count=1&language=cs&format=json&name=" +
               urlEncode(trimmed);

  if (!http.begin(client, url)) {
    Serial.println("[LOC] ERROR: HTTP begin failed");
    return false;
  }
  http.setTimeout(10000);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  Serial.print("[LOC] Hledam lokalitu: ");
  Serial.println(trimmed);

  int code = http.GET();
  if (code != 200) {
    Serial.print("[LOC] ERROR: geocoding HTTP GET selhalo, kod=");
    Serial.println(code);
    http.end();
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();

  if (err) {
    Serial.print("[LOC] ERROR: geocoding JSON parse error: ");
    Serial.println(err.c_str());
    return false;
  }

  if (!doc["results"].is<JsonArray>() || doc["results"].size() == 0) {
    Serial.print("[LOC] Misto nenalezeno: ");
    Serial.println(trimmed);
    return false;
  }

  JsonObject first = doc["results"][0];
  const char* foundName = first["name"] | "";
  float foundLat = first["latitude"] | 0.0f;
  float foundLon = first["longitude"] | 0.0f;

  if (strlen(foundName) == 0) {
    Serial.println("[LOC] ERROR: geocoding odpoved bez nazvu mista");
    return false;
  }

  // Displej hacky/carky neumi (viz stripDiacritics vyse) - geokodovaci
  // dotaz uz ale probehl s puvodni diakritikou (presnejsi shoda v Open-Meteo).
  g_name = stripDiacritics(String(foundName));
  g_lat = foundLat;
  g_lon = foundLon;
  g_configured = true;

  Serial.print("[LOC] Nalezeno: ");
  Serial.print(g_name);
  Serial.print(" (");
  Serial.print(g_lat, 4);
  Serial.print(", ");
  Serial.print(g_lon, 4);
  Serial.println(")");

  if (!writeToDisk()) {
    Serial.println("[LOC] WARNING: lokalita nalezena, ale ulozeni na flash selhalo - plati jen do restartu");
  }
  return true;
}

} // namespace LocationConfig
