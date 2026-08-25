#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>
#include <time.h>

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

// Ikonka vetru 16x10
const unsigned char icon_wind[] PROGMEM = {
  0x00, 0x00, 0xC0, 0x01, 0x20, 0x02, 0x20, 0x02, 0xE0, 0x01,
  0xFC, 0x07, 0x00, 0x00, 0xF8, 0x03, 0x04, 0x04, 0xF8, 0x03
};

// Ikonka kapky 8x12
const unsigned char icon_drop[] PROGMEM = {
  0x10, 0x10, 0x28, 0x28, 0x44, 0x44, 0x82, 0x82, 0x82, 0x44, 0x38, 0x00
};

// Piny pro displej - SPI
#define CS_PIN 4     // D2 (GPIO4)  - CS
#define RST_PIN 5    // D1 (GPIO5)  - RESET
#define DC_PIN 0     // D3 (GPIO0)  - DC
#define CLK_PIN 14   // D5 (GPIO14) - CLK
#define DATA_PIN 13  // D7 (GPIO13) - DATA (MOSI)

// Inicializace displeje
U8G2_SH1106_128X64_NONAME_F_4W_SW_SPI display(U8G2_R0, CLK_PIN, DATA_PIN, CS_PIN, DC_PIN, RST_PIN);

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
int displayMode = 0;  // 0=aktualni, 1=zitra, 2=pozitri
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
    Serial.println("CHYBA: Spatny HTTP kod");
    http.end();
    dataValid = false;
    return;
  }

  String payload = http.getString();
  Serial.println("Data prijata");

  DynamicJsonDocument doc(4096);
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

// ========== KRESLENI NA DISPLEJ ==========

void drawHeader() {
  display.setFont(u8g2_font_7x14B_tf);
  display.drawStr(0, 12, LOCATION_NAME);

  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(85, 12, getLocalTime().c_str());

  display.drawHLine(0, 15, 128);
}

void drawCurrent() {
  drawHeader();

  if (!dataValid) {
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(15, 35, "Nacitam data...");
    display.drawStr(20, 50, lastError.c_str());
    return;
  }

  // Ikonka vlevo
  display.drawXBMP(2, 20, 24, 24, getWeatherIcon(weatherCode));

  // Teplota velka
  display.setFont(u8g2_font_logisoso22_tn);
  display.setCursor(32, 44);
  if (currentTemp >= 0 && currentTemp < 10) display.print(" ");
  display.print(currentTemp, 1);

  // Stupne
  display.drawCircle(100, 24, 2, U8G2_DRAW_ALL);
  display.setFont(u8g2_font_7x14B_tf);
  display.drawStr(104, 38, "C");

  // Popis pocasi
  display.setFont(u8g2_font_6x10_tf);
  String status = getWeatherStatus(weatherCode);
  int w = status.length() * 6;
  display.drawStr((128 - w) / 2, 62, status.c_str());

  display.drawHLine(20, 50, 88);
}

void drawDetails() {
  drawHeader();

  if (!dataValid) {
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(15, 40, "Cekam na data...");
    return;
  }

  // Vlhkost
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(5, 28, "Vlhkost:");
  display.setFont(u8g2_font_7x14B_tf);
  display.setCursor(60, 30);
  display.print((int)humidity);
  display.print(" %");

  // Vitr
  display.drawXBMP(5, 34, 16, 10, icon_wind);
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(25, 44, "Vitr:");
  display.setFont(u8g2_font_7x14B_tf);
  display.setCursor(55, 46);
  display.print((int)windSpeed);
  display.print("km/h ");
  display.setFont(u8g2_font_6x10_tf);
  display.print(getWindDir(windDir));

  // Srazky
  display.drawXBMP(5, 50, 8, 12, icon_drop);
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(18, 60, "Srazky:");
  display.setFont(u8g2_font_7x14B_tf);
  display.setCursor(65, 62);
  display.print(precipProb);
  display.print(" %");
}

void drawForecast(int dayIndex) {
  if (!dataValid) {
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(15, 35, "Cekam na data...");
    return;
  }

  const char* dayName = (dayIndex == 0) ? "ZITRA" : "POZITRI";

  // Nadpis dne
  display.setFont(u8g2_font_7x14B_tf);
  display.drawStr(40, 18, dayName);

  // Ikonka vlevo
  display.drawXBMP(8, 22, 24, 24, getWeatherIcon(forecastCode[dayIndex]));

  // Teploty
  display.setFont(u8g2_font_7x14B_tf);
  display.setCursor(40, 33);
  display.print((int)forecastMin[dayIndex]);
  display.print(" / ");
  display.print((int)forecastMax[dayIndex]);
  display.drawCircle(100, 23, 2, U8G2_DRAW_ALL);
  display.drawStr(104, 33, "C");

  // Popis pocasi
  display.setFont(u8g2_font_6x10_tf);
  String status = getWeatherShort(forecastCode[dayIndex]);
  display.drawStr(40, 48, status.c_str());

  // Horizontalni cara
  display.drawHLine(20, 52, 88);

  // Doplnkove info - vitr a srazky dole
  display.setFont(u8g2_font_5x7_tf);
  display.drawXBMP(10, 55, 16, 10, icon_wind);
  display.setCursor(28, 63);
  display.print((int)forecastWind[dayIndex]);
  display.print("km/h");

  display.drawXBMP(75, 54, 8, 12, icon_drop);
  display.setCursor(86, 63);
  display.print(forecastPrecip[dayIndex]);
  display.print("%");
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== Meteostanice 1-Display ===");

  // Inicializace displeje
  Serial.println("Init displej...");
  display.begin();
  delay(10);

  // Uvodni obrazovka
  display.clearBuffer();
  display.drawRFrame(0, 0, 128, 64, 4);
  display.drawXBMP(8, 18, 24, 24, icon_sun);
  display.setFont(u8g2_font_7x14B_tf);
  display.drawStr(40, 26, "METEO");
  display.drawStr(40, 42, "STANICE");
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(48, 58, LOCATION_NAME);
  display.sendBuffer();

  delay(2000);

  // WiFi pripojeni
  display.clearBuffer();
  display.drawRFrame(0, 0, 128, 64, 4);
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(10, 28, "Pripojuji WiFi...");
  display.drawStr(5, 45, "AP: MeteoStation_AP");
  display.sendBuffer();

  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  if (!wm.autoConnect("MeteoStation_AP")) {
    display.clearBuffer();
    display.setFont(u8g2_font_7x14B_tf);
    display.drawStr(15, 35, "WiFi CHYBA!");
    display.sendBuffer();
    delay(3000);
    ESP.restart();
  }

  // Nastaveni casove zony pro automaticky letni cas
  configTime(TIMEZONE, "pool.ntp.org", "time.nist.gov");

  // Pripojeno
  display.clearBuffer();
  display.drawRFrame(0, 0, 128, 64, 4);
  display.setFont(u8g2_font_7x14B_tf);
  display.drawStr(15, 28, "PRIPOJENO!");
  display.setFont(u8g2_font_6x10_tf);
  display.setCursor(20, 48);
  display.print(WiFi.localIP());
  display.sendBuffer();

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
}

void loop() {
  // Aktualizace casu
  timeClient.update();

  // Aktualizace pocasi kazdych 15 minut
  if (millis() - lastUpdate > 900000) {
    updateWeather();
  }

  // Prepinani obrazovek kazdych 6 sekund
  // 0 = aktualni teplota, 1 = detaily, 2 = zitra, 3 = pozitri
  if (millis() - lastSwitch > 6000) {
    displayMode = (displayMode + 1) % 4;
    lastSwitch = millis();
  }

  // Kresleni na displej
  display.clearBuffer();
  switch (displayMode) {
    case 0:
      drawCurrent();
      break;
    case 1:
      drawDetails();
      break;
    case 2:
      drawForecast(0);  // Zitra
      break;
    case 3:
      drawForecast(1);  // Pozitri
      break;
  }
  display.sendBuffer();

  delay(100);
}
