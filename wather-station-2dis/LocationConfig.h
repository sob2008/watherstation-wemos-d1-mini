#pragma once
// Perzistentni konfigurace lokality zakaznika (nazev + souradnice), ulozena
// v LittleFS pod /config/location.json - prezije i budouci OTA aktualizace
// (ty se tykaji jen aplikacni oblasti flash, ne filesystemu).
//
// Zakaznik zadava misto bydliste jako proste jmeno obce/mesta (do stejneho
// WiFiManager formulare, kde jiz zadava WiFi) - LocationConfig::geocodeAndSave()
// ho pak sam prevede na souradnice pres Open-Meteo Geocoding API (stejny
// poskytovatel jako pocasi, zadna nova zavislost).

#include <Arduino.h>

namespace LocationConfig {

// Pripoji LittleFS (pokud jeste neni) a nacte ulozenou lokalitu. Bezpecne
// volat i kdyz LittleFS uz pripojil jiny modul (LittleFS.begin() je
// idempotentni). Pokud zadna ulozena lokalita neexistuje, zustavaji v
// platnosti vychozi hodnoty (viz LocationConfig.cpp).
void begin();

// True, pokud byla lokalita nekdy uspesne nastavena/ulozena (na rozdil od
// pouhych vychozich hodnot).
bool isConfigured();

// Reference na interni ulozenou hodnotu (ne kopie) - name() se vola i z
// vykreslovacich funkci pri kazdem prekresleni displeje, nema smysl tam
// pri kazdem volani alokovat novy String.
const String& name();
float lat();
float lon();

// Zavola Open-Meteo Geocoding API pro "query" (napr. "Rojetin" nebo
// "Rojetin, okres Sumperk"). Pri uspechu ulozi prvni nalezeny vysledek
// trvale do LittleFS a aktualizuje name()/lat()/lon(). Pri chybe (sit,
// zadny vysledek, spatna odpoved) vraci false a puvodni hodnoty zustavaji
// nezmeneny - volajici by mel v tom pripade pokracovat s tim, co uz je
// (predchozi nebo vychozi), ne zarizeni zablokovat.
bool geocodeAndSave(const String& query);

} // namespace LocationConfig
