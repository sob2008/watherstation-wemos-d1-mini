// Host-side test (g++), mimo slozku sketche.
// Overuje algoritmus odstraneni ceske diakritiky pouzity v
// wather-station-2dis/LocationConfig.cpp (stripDiacritics) - zde je logika
// zkopirovana nad std::string, protoze samotny LocationConfig.cpp zavisi na
// Arduino String/LittleFS a nejde zkompilovat mimo sketch.
//
// POZOR pri praci s timto souborem: C++ escape \xNN je "hladovy" a sezere i
// nasledujici hex-cifry (napr. \x8c + 'e' by se chybne spojilo do \x8ce),
// proto jsou testovaci retezce rozdelene na oddelene literaly "\xNN" "text" -
// kazdy \xNN je vlastni retezcovy literal, spojeny az string-literal
// concatenation (viz komentare u check() volani nize).
#include <string>
#include <cstdint>
#include <cstdio>

struct DiacriticMap { uint8_t b1; uint8_t b2; char ascii; };
const DiacriticMap kDiacritics[] = {
  {0xC3, 0x81, 'A'}, {0xC3, 0xA1, 'a'},
  {0xC4, 0x8C, 'C'}, {0xC4, 0x8D, 'c'},
  {0xC4, 0x8E, 'D'}, {0xC4, 0x8F, 'd'},
  {0xC3, 0x89, 'E'}, {0xC3, 0xA9, 'e'},
  {0xC4, 0x9A, 'E'}, {0xC4, 0x9B, 'e'},
  {0xC3, 0x8D, 'I'}, {0xC3, 0xAD, 'i'},
  {0xC5, 0x87, 'N'}, {0xC5, 0x88, 'n'},
  {0xC3, 0x93, 'O'}, {0xC3, 0xB3, 'o'},
  {0xC5, 0x98, 'R'}, {0xC5, 0x99, 'r'},
  {0xC5, 0xA0, 'S'}, {0xC5, 0xA1, 's'},
  {0xC5, 0xA4, 'T'}, {0xC5, 0xA5, 't'},
  {0xC3, 0x9A, 'U'}, {0xC3, 0xBA, 'u'},
  {0xC5, 0xAE, 'U'}, {0xC5, 0xAF, 'u'},
  {0xC3, 0x9D, 'Y'}, {0xC3, 0xBD, 'y'},
  {0xC5, 0xBD, 'Z'}, {0xC5, 0xBE, 'z'},
};

std::string stripDiacritics(const std::string& utf8) {
  std::string out;
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
      i += 3;
    } else if ((b1 & 0xF8) == 0xF0 && i + 3 < len) {
      i += 4;
    } else {
      i += 1;
    }
  }
  return out;
}

static int failures = 0;
static void check(const char* input, const char* expected) {
  std::string got = stripDiacritics(input);
  bool ok = got == expected;
  printf("[%s] \"%s\" -> \"%s\" (expected \"%s\")\n", ok ? "OK" : "FAIL", input, got.c_str(), expected);
  if (!ok) failures++;
}

int main() {
  check("Rojet" "\xc3" "\xad" "n", "Rojetin");                                              // Rojetin
  check("\xc5" "\xbd" "\xc4" "\x8f" "\xc3" "\xa1" "r nad S" "\xc3" "\xa1" "zavou", "Zdar nad Sazavou"); // Zdar nad Sazavou
  check("\xc4" "\x8c" "esk" "\xc3" "\xbd" " Krumlov", "Cesky Krumlov");                      // Cesky Krumlov
  check("P" "\xc5" "\x99" "erov", "Prerov");                                                 // Prerov
  check("Praha", "Praha");
  check("", "");
  check("\xc5" "\xa0" "umperk", "Sumperk");                                                  // Sumperk
  check("T" "\xc5" "\x99" "eb" "\xc3" "\xad" "\xc4" "\x8d", "Trebic");                        // Trebic
  check("\xc5" "\xbd" "\xc3" "\xa1" "r nad S" "\xc3" "\xa1" "zavou", "Zar nad Sazavou");      // pouze diakriticka pismena bez d s hackem zvlast (kontrola sousednich znaku)

  if (failures == 0) {
    printf("\nVSECHNY TESTY PROSLY\n");
    return 0;
  }
  printf("\n%d TEST(U) SELHALO\n", failures);
  return 1;
}
