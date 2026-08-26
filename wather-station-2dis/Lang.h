#pragma once
// Prepinani jazyka zobrazovaneho textu (cestina/anglictina), perzistentne
// ulozene v LittleFS pod /config/language.json - stejny vzor jako
// LocationConfig. Zakaznik jazyk zada jako "cs"/"en" do stejneho
// WiFiManager formulare, kde zadava WiFi a obec (viz .ino, setup()).
//
// Vsechny retezce jsou i v anglictine bez diakritiky (fonty pouzite na
// displeji ceske hacky/krouzek neumi - viz LocationConfig.cpp,
// stripDiacritics - proto se i cesky zadany nazev lokality na displeji
// zobrazuje bez diakritiky).

#include <Arduino.h>

enum class LangId {
  // --- Pocasi: dlouhy popis (getWeatherStatus) ---
  WeatherClear,
  WeatherMostlyClear,
  WeatherPartlyCloudy,
  WeatherCloudy,
  WeatherFog,
  WeatherLightDrizzle,
  WeatherDrizzle,
  WeatherHeavyDrizzle,
  WeatherFreezingDrizzle,
  WeatherLightRain,
  WeatherRain,
  WeatherHeavyRain,
  WeatherFreezingRain,
  WeatherLightSnow,
  WeatherSnow,
  WeatherHeavySnow,
  WeatherSnowGrains,
  WeatherLightShowers,
  WeatherShowers,
  WeatherHeavyShowers,
  WeatherSnowShowers,
  WeatherThunderstorm,
  WeatherThunderstormHail,

  // --- Pocasi: kratky popis (getWeatherShort, pro predpoved) ---
  WeatherShortClear,
  WeatherShortPartlyCloudy,
  WeatherShortCloudy,
  WeatherShortFog,
  WeatherShortDrizzle,
  WeatherShortFreezingDrizzle,
  WeatherShortRain,
  WeatherShortFreezingRain,
  WeatherShortSnow,
  WeatherShortSnowGrains,
  WeatherShortShowers,
  WeatherShortSnowShowers,
  WeatherShortThunderstorm,
  WeatherShortUnknown,

  // --- Smery vetru (getWindDir) ---
  WindN, WindNE, WindE, WindSE, WindS, WindSW, WindW, WindNW,

  // --- Hlavni obrazovky pocasi ---
  LoadingData,          // "Nacitam data..."
  WaitingForData,       // "Cekam na data..."
  Loading,              // "Nacitam..."
  Tomorrow,             // "ZITRA"
  DayAfterTomorrow,     // "POZITRI"
  Humidity,             // "Vlhkost:"
  Wind,                 // "Vitr:"
  Precipitation,        // "Srazky:"

  // --- Splash / WiFi setup (wather-station-2dis.ino) ---
  MeteoLine1,           // "METEO"
  MeteoLine2,           // "STANICE"
  SecondaryDisplay1,    // "Sekundarni"
  SecondaryDisplay2,    // "displej"
  VersionPrefix,        // "verze"
  ConnectingWifi,       // "Pripojuji WiFi..."
  WaitingForWifi,       // "Cekam na WiFi"
  WifiError,            // "WiFi CHYBA!"
  SearchingLocation,    // "Hledam lokalitu:"
  Connected,            // "PRIPOJENO!"
  WifiOk,               // "WiFi OK!"

  // --- factory-sw.ino ---
  FactoryWelcome,        // "Vitejte!"
  FactoryPleaseWait,      // "Chvilku strpeni"
  FactoryStep1,           // "KROK 1/2"
  FactoryConnectToWifi,   // "Pripojte se na WiFi:"
  FactoryThenEnter,       // "Pak zadejte"
  FactoryWifiAndCity,     // "WiFi + obec"
  FactoryRetrying,        // "Zkousim znovu..."
  FactoryWifiConnected,   // "WIFI PRIPOJENO"
  FactoryConnectedExcl,   // "Pripojeno!"
  FactoryLocation,        // "LOKALITA"
  FactorySearching,       // "Hledam:"
  FactoryNotFound,        // "Nenalezena,"
  FactoryUsingDefault,    // "pouzivam vychozi"
  FactoryFound,           // "Nalezena:"
  FactoryStep2,           // "KROK 2/2"
  FactoryInstalling,      // "Instaluji aktualni"
  FactorySoftware,        // "software..."
  FactoryPleaseWaitCaps,  // "Prosim cekejte"
  FactoryDoNotUnplug,     // "Nevypinejte napajeni"
  FactoryInstallingTitle, // "INSTALUJI SOFTWARE" (titulek OTA prubehu)

  kLangIdCount
};

namespace Lang {

// Pripoji LittleFS (pokud jeste neni) a nacte ulozeny jazyk. Bezpecne volat
// i kdyz LittleFS uz pripojil jiny modul.
void begin();

// "cs" (vychozi) nebo "en" - jine hodnoty se tise ignoruji (zustava puvodni
// jazyk). Trvale uklada do LittleFS.
void setLanguage(const String& code);

bool isEnglish();
const char* code(); // "cs" nebo "en"

// Vrati text pro dany LangId v aktualne nastavenem jazyce.
const char* t(LangId id);

} // namespace Lang
