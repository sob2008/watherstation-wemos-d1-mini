#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>
#include <time.h>

#include "OtaConfig.h"
#include "OtaState.h"
#include "OtaManager.h"

// --- KONFIGURACE ---
const char* LOCATION_NAME = "ROJETIN";  // Nazev lokality na displeji
const float LAT = 49.36;  // Zemente na svou zemepisnou sirku
const float LON = 16.26;  // Zemente na svou zemepisnou delku
const char* TIMEZONE = "CET-1CEST,M3.5.0,M10.5.0/3"; // Automaticky letni cas

// === IKONKY POCASI 24x24 px - XBM format (LSB first) ===

// Slunce
const unsigned char icon_sun[] PROGMEM = {
  0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x40, 0x18, 0x02, 0xC0, 0x00, 0x03,
  0x80, 0x81, 0x01, 0x00, 0xC3, 0x00, 0x00, 0x7E, 0x00, 0x00, 0xFF, 0x00,
  0x80, 0xFF, 0x01, 0xC0, 0xFF, 0x03, 0xC0, 0xFF, 0x03, 0xC7, 0xFF, 0xE3,
  0xC7, 0xFF, 0xE3, 0xC0, 0xFF, 0x03, 0xC0, 0xFF, 0x03, 0x80, 0xFF, 0x01,
  0x00, 0xFF, 0x00, 0x00, 0x7E, 0x00, 0x00, 0xC3, 0x00, 0x80, 0x81, 0x01,
  0xC0, 0x00, 0x03, 0x40, 0x18, 0x02, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00
};

// Polojasno
const unsigned char icon_partcloud[] PROGMEM = {
  0x00, 0x80, 0x01, 0x00, 0x84, 0x01, 0x00, 0x08, 0x11, 0x00, 0xF0, 0x00,
  0x00, 0xF8, 0x21, 0x00, 0xFC, 0x03, 0x00, 0xFC, 0x03, 0x80, 0x9F, 0x01,
  0xC0, 0x3F, 0x00, 0xE0, 0x60, 0x00, 0x70, 0xC0, 0x00, 0x38, 0x80, 0x01,
  0x18, 0x00, 0x03, 0x1C, 0x00, 0x03, 0x0C, 0x00, 0x06, 0x0C, 0x00, 0x06,
  0x0C, 0x00, 0x06, 0x0C, 0x00, 0x06, 0x1C, 0x00, 0x03, 0x18, 0x00, 0x03,
  0x38, 0x80, 0x01, 0xF0, 0xFF, 0x00, 0xE0, 0x7F, 0x00, 0x00, 0x00, 0x00
};

// Mrak
const unsigned char icon_cloud[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x00,
  0x00, 0x3F, 0x00, 0x80, 0x7F, 0x00, 0xC0, 0xFF, 0x00, 0xC0, 0xE1, 0x01,
  0xE0, 0xC0, 0x03, 0xF0, 0x80, 0x03, 0x78, 0x00, 0x07, 0x38, 0x00, 0x0E,
  0x1C, 0x00, 0x1C, 0x1C, 0x00, 0x1C, 0x0C, 0x00, 0x18, 0x0C, 0x00, 0x18,
  0x0C, 0x00, 0x18, 0x1C, 0x00, 0x1C, 0x18, 0x00, 0x0C, 0x38, 0x00, 0x0E,
  0xF0, 0xFF, 0x07, 0xE0, 0xFF, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Dest
const unsigned char icon_rain[] PROGMEM = {
  0x00, 0x1E, 0x00, 0x00, 0x3F, 0x00, 0x80, 0x7F, 0x00, 0xC0, 0xE1, 0x01,
  0xE0, 0xC0, 0x03, 0x70, 0x80, 0x03, 0x38, 0x00, 0x07, 0x1C, 0x00, 0x0E,
  0x0C, 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x1C, 0x00, 0x0E,
  0xF8, 0xFF, 0x07, 0xF0, 0xFF, 0x03, 0x00, 0x00, 0x00, 0x20, 0x82, 0x00,
  0x10, 0x41, 0x08, 0x10, 0x41, 0x08, 0x88, 0x20, 0x04, 0x00, 0x00, 0x00,
  0x20, 0x82, 0x00, 0x10, 0x41, 0x08, 0x10, 0x41, 0x08, 0x00, 0x00, 0x00
};

// Snih
const unsigned char icon_snow[] PROGMEM = {
  0x00, 0x1E, 0x00, 0x00, 0x3F, 0x00, 0x80, 0x7F, 0x00, 0xC0, 0xE1, 0x01,
  0xE0, 0xC0, 0x03, 0x70, 0x80, 0x03, 0x38, 0x00, 0x07, 0x1C, 0x00, 0x0E,
  0x0C, 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x1C, 0x00, 0x0E,
  0xF8, 0xFF, 0x07, 0xF0, 0xFF, 0x03, 0x00, 0x00, 0x00, 0x00, 0x41, 0x00,
  0xA0, 0xA2, 0x02, 0x40, 0x14, 0x01, 0xA0, 0xA2, 0x02, 0x00, 0x41, 0x00,
  0x40, 0x14, 0x01, 0xA0, 0xA2, 0x02, 0x00, 0x41, 0x00, 0x00, 0x00, 0x00
};

// Boure
const unsigned char icon_storm[] PROGMEM = {
  0x00, 0x1E, 0x00, 0x00, 0x3F, 0x00, 0x80, 0x7F, 0x00, 0xC0, 0xE1, 0x01,
  0xE0, 0xC0, 0x03, 0x70, 0x80, 0x03, 0x38, 0x00, 0x07, 0x1C, 0x00, 0x0E,
  0x0C, 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x1C, 0x00, 0x0E, 0xF8, 0xFF, 0x07,
  0xF0, 0xFF, 0x03, 0x00, 0x0C, 0x00, 0x00, 0x06, 0x00, 0x00, 0x03, 0x00,
  0x80, 0x3F, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x07, 0x00,
  0x00, 0x03, 0x00, 0x80, 0x01, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00
};

// === MALE IKONKY 16x16 ===

const unsigned char icon_sun_sm[] PROGMEM = {
  0x80, 0x01, 0x80, 0x01, 0x24, 0x24, 0x18, 0x18, 0xE0, 0x07, 0xF0, 0x0F,
  0xF8, 0x1F, 0xFB, 0xDF, 0xFB, 0xDF, 0xF8, 0x1F, 0xF0, 0x0F, 0xE0, 0x07,
  0x18, 0x18, 0x24, 0x24, 0x80, 0x01, 0x80, 0x01
};

const unsigned char icon_partcloud_sm[] PROGMEM = {
  0x00, 0x06, 0x00, 0x06, 0x80, 0x4F, 0xC0, 0x1F, 0xE0, 0x07, 0xF0, 0x03,
  0xF8, 0x01, 0x18, 0x03, 0x0C, 0x06, 0x06, 0x0C, 0x06, 0x0C, 0x06, 0x0C,
  0x06, 0x06, 0xFE, 0x07, 0xFC, 0x03, 0x00, 0x00
};

const unsigned char icon_cloud_sm[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0xE0, 0x01, 0xF0, 0x03, 0x38, 0x07, 0x18, 0x06,
  0x0C, 0x0C, 0x06, 0x18, 0x06, 0x18, 0x06, 0x18, 0x06, 0x18, 0x06, 0x0C,
  0xFE, 0x0F, 0xFC, 0x07, 0x00, 0x00, 0x00, 0x00
};

const unsigned char icon_rain_sm[] PROGMEM = {
  0xE0, 0x01, 0xF0, 0x03, 0x38, 0x07, 0x0C, 0x0C, 0x06, 0x18, 0x06, 0x18,
  0x06, 0x0C, 0xFE, 0x0F, 0xFC, 0x07, 0x00, 0x00, 0x44, 0x04, 0x22, 0x02,
  0x44, 0x04, 0x22, 0x02, 0x44, 0x04, 0x00, 0x00
};

const unsigned char icon_snow_sm[] PROGMEM = {
  0xE0, 0x01, 0xF0, 0x03, 0x38, 0x07, 0x0C, 0x0C, 0x06, 0x18, 0x06, 0x18,
  0x06, 0x0C, 0xFE, 0x0F, 0xFC, 0x07, 0x00, 0x00, 0x88, 0x08, 0x50, 0x05,
  0x88, 0x08, 0x50, 0x05, 0x88, 0x08, 0x00, 0x00
};

const unsigned char icon_storm_sm[] PROGMEM = {
  0xE0, 0x01, 0xF0, 0x03, 0x38, 0x07, 0x0C, 0x0C, 0x06, 0x18, 0x06, 0x0C,
  0xFE, 0x0F, 0xFC, 0x07, 0x80, 0x01, 0xC0, 0x00, 0xE0, 0x03, 0x80, 0x01,
  0xC0, 0x00, 0x60, 0x00, 0x20, 0x00, 0x00, 0x00
};

// === IKONKY WiFi 12x10 ===
const unsigned char icon_wifi_4[] PROGMEM = { // plny signal
  0xF8, 0x01, 0x0E, 0x07, 0xE2, 0x04, 0x38, 0x01, 0x44, 0x02,
  0xE0, 0x00, 0x10, 0x00, 0x60, 0x00, 0x60, 0x00, 0x00, 0x00
};
const unsigned char icon_wifi_3[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0xE0, 0x00, 0x38, 0x01, 0x44, 0x02,
  0xE0, 0x00, 0x10, 0x00, 0x60, 0x00, 0x60, 0x00, 0x00, 0x00
};
const unsigned char icon_wifi_2[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00,
  0xE0, 0x00, 0x10, 0x00, 0x60, 0x00, 0x60, 0x00, 0x00, 0x00
};
const unsigned char icon_wifi_1[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x60, 0x00, 0x00, 0x00
};

// Ikonka vetru 16x10
const unsigned char icon_wind[] PROGMEM = {
  0x00, 0x00, 0xC0, 0x01, 0x20, 0x02, 0x20, 0x02, 0xE0, 0x01,
  0xFC, 0x07, 0x00, 0x00, 0xF8, 0x03, 0x04, 0x04, 0xF8, 0x03
};

// Ikonka kapky 8x12
const unsigned char icon_drop[] PROGMEM = {
  0x10, 0x10, 0x28, 0x28, 0x44, 0x44, 0x82, 0x82, 0x82, 0x44, 0x38, 0x00
};

// Piny pro displeje - KAZDY MA VLASTNI CS A RESET!
#define CS1_PIN 4    // D2 (GPIO4)  - Displej 1 CS
#define CS2_PIN 12   // D6 (GPIO12) - Displej 2 CS
#define RST1_PIN 5   // D1 (GPIO5)  - Displej 1 RESET
#define RST2_PIN 2   // D4 (GPIO2)  - Displej 2 RESET
#define DC_PIN 0     // D3 (GPIO0)  - Sdileny DC
#define CLK_PIN 14   // D5 (GPIO14) - Sdileny CLK
#define DATA_PIN 13  // D7 (GPIO13) - Sdileny DATA

// Inicializace displeju - kazdy ma vlastni CS a RESET
U8G2_SH1106_128X64_NONAME_F_4W_SW_SPI disp1(U8G2_R0, CLK_PIN, DATA_PIN, CS1_PIN, DC_PIN, RST1_PIN);
U8G2_SH1106_128X64_NONAME_F_4W_SW_SPI disp2(U8G2_R0, CLK_PIN, DATA_PIN, CS2_PIN, DC_PIN, RST2_PIN);

// Cas
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

// Data pocasi
float currentTemp = -999;
int weatherCode = -1;
float humidity = 0;
float windSpeed = 0;
int windDir = 0;
int precipProb = 0;

// Predpoved
float forecastMin[2] = {-999, -999};
float forecastMax[2] = {-999, -999};
int forecastCode[2] = {-1, -1};
int forecastPrecip[2] = {0, 0};
float forecastWind[2] = {0, 0};

// Stav
unsigned long lastUpdate = 0;
bool showForecast = false;
unsigned long lastSwitch = 0;
bool dataValid = false;
String lastError = "";

// Funkce pro prevod kodu pocasi na text
String getWeatherStatus(int code) {
  switch (code) {
    case 0: return "Jasno";
    case 1: return "Skoro jasno";
    case 2: return "Polojasno";
    case 3: return "Zatazeno";
    case 45: case 48: return "Mlha";
    case 51: return "Slabe mrholeni";
    case 53: return "Mrholeni";
    case 55: return "Silne mrholeni";
    case 56: case 57: return "Mraz. mrholeni";
    case 61: return "Slaby dest";
    case 63: return "Dest";
    case 65: return "Silny dest";
    case 66: case 67: return "Mraz. dest";
    case 71: return "Slabe snezeni";
    case 73: return "Snezeni";
    case 75: return "Silne snezeni";
    case 77: return "Sneh. zrna";
    case 80: return "Slaba preh.";
    case 81: return "Prehanka";
    case 82: return "Silna preh.";
    case 85: case 86: return "Sneh. preh.";
    case 95: return "Boure";
    case 96: case 99: return "Boure s krup.";
    default: return "Zatazeno";
  }
}

// Kratsi verze pro predpoved
String getWeatherShort(int code) {
  switch (code) {
    case 0: return "Jasno";
    case 1: case 2: return "Polojasno";
    case 3: return "Zatazeno";
    case 45: case 48: return "Mlha";
    case 51: case 53: case 55: return "Mrholeni";
    case 56: case 57: return "Mrz.mrh.";
    case 61: case 63: case 65: return "Dest";
    case 66: case 67: return "Mrz.dest";
    case 71: case 73: case 75: return "Snezeni";
    case 77: return "Sn.zrna";
    case 80: case 81: case 82: return "Prehanky";
    case 85: case 86: return "Sn.preh.";
    case 95: case 96: case 99: return "Boure";
    default: return "---";
  }
}

const unsigned char* getWeatherIcon(int code) {
  if (code == 0) return icon_sun;
  if (code == 1 || code == 2) return icon_partcloud;
  if (code == 3) return icon_cloud;
  if (code == 45 || code == 48) return icon_cloud;
  if (code >= 51 && code <= 57) return icon_rain;
  if (code >= 61 && code <= 67) return icon_rain;
  if (code >= 71 && code <= 77) return icon_snow;
  if (code >= 80 && code <= 82) return icon_rain;
  if (code >= 85 && code <= 86) return icon_snow;
  if (code >= 95 && code <= 99) return icon_storm;
  return icon_cloud;
}

const unsigned char* getWeatherIconSmall(int code) {
  if (code == 0) return icon_sun_sm;
  if (code == 1 || code == 2) return icon_partcloud_sm;
  if (code == 3) return icon_cloud_sm;
  if (code == 45 || code == 48) return icon_cloud_sm;
  if (code >= 51 && code <= 57) return icon_rain_sm;
  if (code >= 61 && code <= 67) return icon_rain_sm;
  if (code >= 71 && code <= 77) return icon_snow_sm;
  if (code >= 80 && code <= 82) return icon_rain_sm;
  if (code >= 85 && code <= 86) return icon_snow_sm;
  if (code >= 95 && code <= 99) return icon_storm_sm;
  return icon_cloud_sm;
}

// Vraci spravnou WiFi ikonku podle sily signalu
const unsigned char* getWifiIcon() {
  int rssi = WiFi.RSSI();
  if (rssi > -50) return icon_wifi_4;
  if (rssi > -60) return icon_wifi_3;
  if (rssi > -70) return icon_wifi_2;
  return icon_wifi_1;
}

// Smer vetru na text
String getWindDir(int deg) {
  if (deg < 23) return "S";
  if (deg < 68) return "SV";
  if (deg < 113) return "V";
  if (deg < 158) return "JV";
  if (deg < 203) return "J";
  if (deg < 248) return "JZ";
  if (deg < 293) return "Z";
  if (deg < 338) return "SZ";
  return "S";
}

// Ziskani casu s automatickym letnim casem
String getLocalTime() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char buffer[6];
  sprintf(buffer, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
  return String(buffer);
}

// Zobrazeni stavu OTA aktualizace na hlavnim displeji (volano z OtaManager
// behem stahovani/flashovani, aby displej behem nekolikasekundove operace
// nepusobil "zamrzle").
void otaStatusCallback(const String& line1, const String& line2) {
  disp1.clearBuffer();
  disp1.drawRFrame(0, 0, 128, 64, 4);
  disp1.setFont(u8g2_font_7x14B_tf);
  disp1.drawStr(10, 28, line1.c_str());
  disp1.setFont(u8g2_font_6x10_tf);
  disp1.drawStr(10, 45, line2.c_str());
  disp1.sendBuffer();
}

void updateWeather() {
  Serial.println("\n=== Aktualizace pocasi ===");

  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient http;

  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(LAT) +
               "&longitude=" + String(LON) +
               "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m,wind_direction_10m" +
               "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max,wind_speed_10m_max" +
               "&timezone=auto&forecast_days=3";

  Serial.println("URL: " + url);

  if (!http.begin(*client, url)) {
    lastError = "HTTP fail";
    Serial.println("CHYBA: HTTP begin selhalo");
    dataValid = false;
    return;
  }

  int httpCode = http.GET();
  Serial.print("HTTP kod: ");
  Serial.println(httpCode);

  if (httpCode != 200) {
    lastError = "HTTP " + String(httpCode);
   Serial.print("HTTP chyba: ");
   Serial.println(http.errorToString(httpCode));
    http.end();
    dataValid = false;
    return;
  }

  String payload = http.getString();
  Serial.println("Data prijata");

  JsonDocument doc;
  DeserializationError jsonErr = deserializeJson(doc, payload);

  if (jsonErr) {
    lastError = "JSON err";
    Serial.print("CHYBA JSON: ");
    Serial.println(jsonErr.c_str());
    http.end();
    dataValid = false;
    return;
  }

  // Aktualni pocasi
  currentTemp = doc["current"]["temperature_2m"];
  weatherCode = doc["current"]["weather_code"];
  humidity = doc["current"]["relative_humidity_2m"];
  windSpeed = doc["current"]["wind_speed_10m"];
  windDir = doc["current"]["wind_direction_10m"];

  // Pravdepodobnost srazek dnes
  precipProb = doc["daily"]["precipitation_probability_max"][0];

  Serial.print("Teplota: "); Serial.println(currentTemp);
  Serial.print("Vlhkost: "); Serial.println(humidity);
  Serial.print("Vitr: "); Serial.print(windSpeed); Serial.print(" km/h "); Serial.println(getWindDir(windDir));
  Serial.print("Srazky: "); Serial.print(precipProb); Serial.println("%");

  // Predpoved na zitra a pozitri
  for (int i = 0; i < 2; i++) {
    forecastMin[i] = doc["daily"]["temperature_2m_min"][i + 1];
    forecastMax[i] = doc["daily"]["temperature_2m_max"][i + 1];
    forecastCode[i] = doc["daily"]["weather_code"][i + 1];
    forecastPrecip[i] = doc["daily"]["precipitation_probability_max"][i + 1];
    forecastWind[i] = doc["daily"]["wind_speed_10m_max"][i + 1];
  }

  http.end();
  lastUpdate = millis();
  dataValid = true;
  lastError = "";
  Serial.println("=== Data OK ===\n");
}

// ========== KRESLENI NA DISPLEJ 1 (hlavni) ==========

void disp1_drawHeader() {
  disp1.setFont(u8g2_font_7x14B_tf);
  disp1.drawStr(0, 12, LOCATION_NAME);

  disp1.setFont(u8g2_font_6x10_tf);
  disp1.drawStr(85, 12, getLocalTime().c_str());

  disp1.drawHLine(0, 15, 128);
}

void disp1_drawCurrent() {
  disp1_drawHeader();

  if (!dataValid) {
    disp1.setFont(u8g2_font_6x10_tf);
    disp1.drawStr(15, 35, "Nacitam data...");
    disp1.drawStr(20, 50, lastError.c_str());
    return;
  }

  // Ikonka vlevo
  disp1.drawXBMP(2, 20, 24, 24, getWeatherIcon(weatherCode));

  // Teplota velka
  disp1.setFont(u8g2_font_logisoso22_tn);
  disp1.setCursor(32, 44);
  if (currentTemp >= 0 && currentTemp < 10) disp1.print(" ");
  disp1.print(currentTemp, 1);

  // Stupne
  disp1.drawCircle(100, 24, 2, U8G2_DRAW_ALL);
  disp1.setFont(u8g2_font_7x14B_tf);
  disp1.drawStr(104, 38, "C");

  // Popis pocasi
  disp1.setFont(u8g2_font_6x10_tf);
  String status = getWeatherStatus(weatherCode);
  int w = status.length() * 6;
  disp1.drawStr((128 - w) / 2, 62, status.c_str());

  disp1.drawHLine(20, 50, 88);
}

void disp1_drawForecast(int dayIndex) {
  if (!dataValid) {
    disp1.setFont(u8g2_font_6x10_tf);
    disp1.drawStr(15, 35, "Cekam na data...");
    return;
  }

  const char* dayName = (dayIndex == 0) ? "ZITRA" : "POZITRI";

  // Nadpis dne - bez banneru, stejny vzhled jako disp2
  disp1.setFont(u8g2_font_7x14B_tf);
  disp1.drawStr(40, 18, dayName);

  // Ikonka vlevo
  disp1.drawXBMP(8, 22, 24, 24, getWeatherIcon(forecastCode[dayIndex]));

  // Teploty
  disp1.setFont(u8g2_font_7x14B_tf);
  disp1.setCursor(40, 33);
  disp1.print((int)forecastMin[dayIndex]);
  disp1.print(" / ");
  disp1.print((int)forecastMax[dayIndex]);
  disp1.drawCircle(100, 23, 2, U8G2_DRAW_ALL);
  disp1.drawStr(104, 33, "C");

  // Popis pocasi
  disp1.setFont(u8g2_font_6x10_tf);
  String status = getWeatherShort(forecastCode[dayIndex]);
  disp1.drawStr(40, 48, status.c_str());

  // Horizontalni cara
  disp1.drawHLine(20, 52, 88);

  // Doplnkove info - vitr a srazky dole
  disp1.setFont(u8g2_font_5x7_tf);
  disp1.drawXBMP(10, 55, 16, 10, icon_wind);
  disp1.setCursor(28, 63);
  disp1.print((int)forecastWind[dayIndex]);
  disp1.print("km/h");

  disp1.drawXBMP(75, 54, 8, 12, icon_drop);
  disp1.setCursor(86, 63);
  disp1.print(forecastPrecip[dayIndex]);
  disp1.print("%");
}

// ========== KRESLENI NA DISPLEJ 2 (sekundarni) ==========

void disp2_drawDetails() {
  if (!dataValid) {
    disp2.setFont(u8g2_font_6x10_tf);
    disp2.drawStr(30, 35, "Nacitam...");
    return;
  }

  // WiFi signal
  disp2.drawXBMP(2, 2, 12, 10, getWifiIcon());
  disp2.setFont(u8g2_font_5x7_tf);
  disp2.setCursor(16, 9);
  disp2.print(WiFi.RSSI());
  disp2.print("dBm");

  // Ramecek
  disp2.drawRFrame(0, 14, 128, 50, 4);

  // Vlhkost
  disp2.setFont(u8g2_font_6x10_tf);
  disp2.drawStr(8, 28, "Vlhkost:");
  disp2.setFont(u8g2_font_7x14B_tf);
  disp2.setCursor(70, 30);
  disp2.print((int)humidity);
  disp2.print(" %");

  // Vitr
  disp2.drawXBMP(8, 34, 16, 10, icon_wind);
  disp2.setFont(u8g2_font_6x10_tf);
  disp2.drawStr(28, 44, "Vitr:");
  disp2.setFont(u8g2_font_7x14B_tf);
  disp2.setCursor(60, 46);
  disp2.print((int)windSpeed);
  disp2.print("km/h ");
  disp2.setFont(u8g2_font_6x10_tf);
  disp2.print(getWindDir(windDir));

  // Srazky
  disp2.drawXBMP(8, 50, 8, 12, icon_drop);
  disp2.setFont(u8g2_font_6x10_tf);
  disp2.drawStr(20, 60, "Srazky:");
  disp2.setFont(u8g2_font_7x14B_tf);
  disp2.setCursor(70, 62);
  disp2.print(precipProb);
  disp2.print(" %");
}

void disp2_drawForecast(int dayIndex) {
  if (!dataValid) {
    disp2.setFont(u8g2_font_6x10_tf);
    disp2.drawStr(30, 35, "Nacitam...");
    return;
  }

  const char* dayName = (dayIndex == 0) ? "ZITRA" : "POZITRI";

  // Nadpis dne - stejne jako disp1
  disp2.setFont(u8g2_font_7x14B_tf);
  disp2.drawStr(40, 18, dayName);

  // Ikonka vlevo - stejne jako disp1
  disp2.drawXBMP(8, 22, 24, 24, getWeatherIcon(forecastCode[dayIndex]));

  // Teploty - stejne jako disp1
  disp2.setFont(u8g2_font_7x14B_tf);
  disp2.setCursor(40, 33);
  disp2.print((int)forecastMin[dayIndex]);
  disp2.print(" / ");
  disp2.print((int)forecastMax[dayIndex]);
  disp2.drawCircle(100, 23, 2, U8G2_DRAW_ALL);
  disp2.drawStr(104, 33, "C");

  // Popis pocasi - stejne jako disp1
  disp2.setFont(u8g2_font_6x10_tf);
  String status = getWeatherShort(forecastCode[dayIndex]);
  disp2.drawStr(40, 48, status.c_str());

  // Horizontalni cara
  disp2.drawHLine(20, 52, 88);

  // Doplnkove info - vitr a srazky dole
  disp2.setFont(u8g2_font_5x7_tf);
  disp2.drawXBMP(10, 55, 16, 10, icon_wind);
  disp2.setCursor(28, 63);
  disp2.print((int)forecastWind[dayIndex]);
  disp2.print("km/h");

  disp2.drawXBMP(75, 54, 8, 12, icon_drop);
  disp2.setCursor(86, 63);
  disp2.print(forecastPrecip[dayIndex]);
  disp2.print("%");
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== Meteostanice 2-Display ===");

  // Inicializace displeje 1
  Serial.println("Init displej 1...");
  disp1.begin();
  delay(10);

  // Inicializace displeje 2
  Serial.println("Init displej 2...");
  disp2.begin();
  delay(10);

  Serial.println("Oba displeje inicializovany");

  // Uvodni obrazovka - Displej 1
  disp1.clearBuffer();
  disp1.drawRFrame(0, 0, 128, 64, 4);
  disp1.drawXBMP(8, 18, 24, 24, icon_sun);
  disp1.setFont(u8g2_font_7x14B_tf);
  disp1.drawStr(40, 26, "METEO");
  disp1.drawStr(40, 42, "STANICE");
  disp1.setFont(u8g2_font_6x10_tf);
  disp1.drawStr(48, 58, LOCATION_NAME);
  disp1.sendBuffer();

  // Uvodni obrazovka - Displej 2
  disp2.clearBuffer();
  disp2.drawRFrame(0, 0, 128, 64, 4);
  disp2.setFont(u8g2_font_6x10_tf);
  disp2.drawStr(25, 25, "Sekundarni");
  disp2.drawStr(35, 40, "displej");
  disp2.setFont(u8g2_font_5x7_tf);
  disp2.drawStr(30, 55, "verze " FIRMWARE_VERSION);
  disp2.sendBuffer();

  delay(2000);

  // OTA: pripojit LittleFS, nacist perzistentni stav a rozhodnout o
  // pripadnem rollbacku na predchozi firmware - vse PRED pripojenim WiFi,
  // aby rollback nezavisel na siti (viz README, sekce OTA).
  OtaState::begin();
  OtaManager::setStatusCallback(otaStatusCallback);
  OtaManager::begin();

  // WiFi pripojeni - Displej 1
  disp1.clearBuffer();
  disp1.drawRFrame(0, 0, 128, 64, 4);
  disp1.setFont(u8g2_font_6x10_tf);
  disp1.drawStr(10, 28, "Pripojuji WiFi...");
  disp1.drawStr(5, 45, "AP: MeteoStation_AP");
  disp1.sendBuffer();

  // WiFi pripojeni - Displej 2
  disp2.clearBuffer();
  disp2.drawRFrame(0, 0, 128, 64, 4);
  disp2.setFont(u8g2_font_6x10_tf);
  disp2.drawStr(25, 35, "Cekam na WiFi");
  disp2.sendBuffer();

  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  if (!wm.autoConnect("MeteoStation_AP")) {
    disp1.clearBuffer();
    disp1.setFont(u8g2_font_7x14B_tf);
    disp1.drawStr(15, 35, "WiFi CHYBA!");
    disp1.sendBuffer();
    delay(3000);
    ESP.restart();
  }

  // Nastaveni casove zony pro automaticky letni cas
  configTime(TIMEZONE, "pool.ntp.org", "time.nist.gov");

  // Pripojeno - Displej 1
  disp1.clearBuffer();
  disp1.drawRFrame(0, 0, 128, 64, 4);
  disp1.setFont(u8g2_font_7x14B_tf);
  disp1.drawStr(15, 28, "PRIPOJENO!");
  disp1.setFont(u8g2_font_6x10_tf);
  disp1.setCursor(20, 48);
  disp1.print(WiFi.localIP());
  disp1.sendBuffer();

  // Pripojeno - Displej 2
  disp2.clearBuffer();
  disp2.drawRFrame(0, 0, 128, 64, 4);
  disp2.setFont(u8g2_font_6x10_tf);
  disp2.drawStr(35, 30, "WiFi OK!");
  disp2.setCursor(25, 48);
  disp2.print("RSSI: ");
  disp2.print(WiFi.RSSI());
  disp2.print(" dBm");
  disp2.sendBuffer();

  delay(1500);

  Serial.println("WiFi pripojeno!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  timeClient.begin();

  // Pockej na synchronizaci casu
  Serial.println("Synchronizuji cas...");
  int attempts = 0;
  while (time(nullptr) < 100000 && attempts < 20) {
    delay(500);
    attempts++;
  }

  Serial.println("Stahuji data...");
  updateWeather();

  // Firmware se dostal az sem bez pádu/watchdog resetu a WiFi funguje -
  // pokud jsme nabootovali z OTA, oznacit tuto verzi za funkcni (viz
  // OtaManager.h - zabranuje rollbacku kvuli slow-start, ne kvuli
  // skutecne vadnemu firmware).
  OtaManager::notifyApplicationHealthy();
}

void loop() {
  // Aktualizace casu
  timeClient.update();

  // OTA: neblokujici mimo aktivni kontrolu (jednou za OTA_CHECK_INTERVAL_MS).
  // Pri nalezeni kompatibilni novejsi verze tento vola blokujici stahovani -
  // displej behem toho ukazuje stav pres otaStatusCallback().
  OtaManager::handle();

  // Aktualizace pocasi kazdych 15 minut
  if (millis() - lastUpdate > 900000) {
    updateWeather();
  }

  // Prepinani obrazovek kazdych 8 sekund
  if (millis() - lastSwitch > 8000) {
    showForecast = !showForecast;
    lastSwitch = millis();
  }

  // Kresleni na displej 1
  disp1.clearBuffer();
  if (!showForecast) {
    disp1_drawCurrent();
  } else {
    disp1_drawForecast(0);  // Zitra
  }
  disp1.sendBuffer();

  // Kresleni na displej 2
  disp2.clearBuffer();
  if (!showForecast) {
    disp2_drawDetails();
  } else {
    disp2_drawForecast(1);  // Pozitri
  }
  disp2.sendBuffer();

  delay(100);
}
