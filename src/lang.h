// Teksty na wyświetlaczu w kilku językach – wszystkie w jednym miejscu.
// Wybrany język jest zapisany w NVS (ustawiany z ekranu konfiguracji).
//
// Nowy język: wartość w Lang (przed COUNT) i jedna kolumna w LANG_STRINGS
// (kolejny argument X w każdym wierszu). Bez polskich znaków – fonty TFT_eSPI ich nie mają.
// Nowy tekst: wiersz w LANG_STRINGS, w kodzie tr(S_...).
#pragma once

#include <stdint.h>

enum class Lang : uint8_t { PL, EN, COUNT };

// X(id, polski, angielski)
#define LANG_STRINGS(X)                                                                              \
  /* ekrany informacyjne (Dashboard) */                                                             \
  X(S_CONNECTING,       "Laczenie z WiFi",            "Connecting to WiFi")                         \
  X(S_WIFI_SETUP,       "Konfiguracja WiFi",          "WiFi setup")                                 \
  X(S_PORTAL_JOIN,      "Polacz sie z siecia:",       "Connect to the network:")                    \
  X(S_PORTAL_PASSWORD,  "haslo: ",                    "password: ")                                 \
  X(S_PORTAL_OPEN,      "i otworz w przegladarce:",   "and open in a browser:")                     \
  X(S_WAITING,          "Czekam na dane z gry",       "Waiting for game data")                      \
  X(S_LAST_GAME,        "ostatnio: ",                 "last: ")                                     \
  X(S_IP,               "IP:",                        "IP:")                                        \
  X(S_PORT,             "port:",                      "port:")                                      \
  X(S_WIFI_ERASING,     "Kasowanie WiFi",             "Erasing WiFi")                               \
  X(S_RESTART_HOTSPOT,  "Restart w trybie hotspot...", "Restarting in hotspot mode...")             \
  X(S_FACTORY_ERASING,  "Kasowanie ustawien",         "Erasing settings")                           \
  X(S_RESTARTING,       "Restart...",                 "Restarting...")                              \
  X(S_OTA_TITLE,        "Aktualizacja OTA",           "OTA update")                                 \
  X(S_OTA_KEEP_POWER,   "Nie wylaczaj zasilania",     "Do not switch the power off")                \
  X(S_OTA_ERROR,        "Blad aktualizacji OTA",      "OTA update failed")                          \
  /* podpowiedzi gier (games/game_list.cpp) */                                                      \
  X(S_HINT_BEAMNG,      "BeamNG: Options > Other > OutGauge", "BeamNG: Options > Other > OutGauge") \
  X(S_HINT_SCS,         "ETS2 / ATS: wtyczka ETS2Mod", "ETS2 / ATS: ETS2Mod plugin")                \
  /* dotyk (Touch) */                                                                               \
  X(S_TOUCH_CALIBRATION, "Kalibracja dotyku",         "Touch calibration")                          \
  X(S_CAL_HINT1,        "Dotykaj po kolei krzyzykow w rogach", "Touch the crosshairs in the corners") \
  X(S_CAL_HINT2,        "(najlepiej rysikiem)",       "(a stylus works best)")                      \
  X(S_TOUCH_TEST,       "Test dotyku",                "Touch test")                                 \
  X(S_TEST_HINT1,       "Dotknij po kolei 4 punktow", "Touch the 4 points in turn")                 \
  X(S_TEST_HINT2,       "Na kazdy masz 5 s",          "5 s for each one")                           \
  X(S_TEST_POINT,       "Punkt ",                     "Point ")                                     \
  X(S_CAL_OK,           "Kalibracja OK",              "Calibration OK")                             \
  X(S_CAL_FAILED,       "Kalibracja nieudana",        "Calibration failed")                         \
  X(S_CAL_SAVED,        "Dane zapisane",              "Data saved")                                 \
  X(S_TOUCH_OFF,        "Dotyk wylaczony",            "Touch disabled")                             \
  X(S_CAL_NEXT_BOOT,    "Kalibracja ruszy przy nastepnym starcie", "Calibration will run at next start") \
  X(S_CAL_KEPT,         "Zostaje poprzednia kalibracja", "Previous calibration kept")               \
  /* ekran konfiguracji */                                                                          \
  X(S_CONFIG_TITLE,     "KONFIGURACJA",               "SETTINGS")                                   \
  X(S_CONFIG_HINT,      "1 OK   2 w gore   3 w dol   4 wyjscie", "1 OK   2 up   3 down   4 exit")  \
  X(S_EXIT,             "Wyjscie",                    "Exit")                                       \
  X(S_BRIGHTNESS_ITEM,  "Poziom jasnosci",            "Brightness")                                 \
  X(S_LANGUAGE_ITEM,    "Jezyk: polski",              "Language: English")                          \
  X(S_FACTORY_RESET,    "Reset urzadzenia",           "Factory reset")                              \
  X(S_CONFIRM_HINT,     "2/3 wybor   1 OK   4 wstecz", "2/3 choose   1 OK   4 back")                \
  X(S_YES,              "Tak",                        "Yes")                                        \
  X(S_NO,               "Nie",                        "No")                                         \
  X(S_WILL_RESTART,     "Urzadzenie sie zrestartuje.", "The device will restart.")                  \
  X(S_WIFI_TITLE,       "KONFIGURACJA WIFI",          "WIFI SETUP")                                 \
  X(S_WIFI_ASK1,        "Skasowac zapisana siec WiFi", "Erase the saved WiFi network")              \
  X(S_WIFI_ASK2,        "i uruchomic hotspot?",       "and start the hotspot?")                     \
  X(S_FACTORY_TITLE,    "RESET URZADZENIA",           "FACTORY RESET")                              \
  X(S_FACTORY_ASK1,     "Skasowac wszystkie ustawienia:", "Erase all settings:")                    \
  X(S_FACTORY_ASK2,     "WiFi, dotyk, jasnosc, jezyk?", "WiFi, touch, brightness, language?")       \
  X(S_BRIGHTNESS_TITLE, "JASNOSC",                    "BRIGHTNESS")                                 \
  X(S_BRIGHTNESS_HINT,  "2 ciemniej   3 jasniej   1 zapisz   4 anuluj", "2 darker   3 brighter   1 save   4 cancel") \
  X(S_CANCEL,           "Anuluj",                     "Cancel")                                     \
  X(S_SAVE,             "Zapisz",                     "Save")                                       \
  /* ekrany auta */                                                                                 \
  X(S_FUEL,             "Paliwo",                     "Fuel")                                       \
  X(S_ENGINE,           "Silnik",                     "Engine")                                     \
  X(S_OIL,              "Olej",                       "Oil")                                        \
  X(S_TURBO,            "Turbo",                      "Turbo")                                      \
  X(S_THROTTLE,         "Gaz",                        "Throttle")                                   \
  X(S_BRAKE,            "Hamulec",                    "Brake")                                      \
  X(S_THROTTLE_SHORT,   "Gaz",                        "Thr.")                                       \
  X(S_BRAKE_SHORT,      "Ham.",                       "Brk.")                                       \
  X(S_CLUTCH_SHORT,     "Sprz.",                      "Clu.")                                       \
  X(S_GRAPH_THROTTLE,   "gaz",                        "thr.")                                       \
  X(S_GRAPH_BRAKE,      "hamulec",                    "brake")                                      \
  X(S_ENGINE_TITLE,     "SILNIK",                     "ENGINE")                                     \
  X(S_ENGINE_TEMP,      "Temp. silnika",              "Engine temp.")                               \
  X(S_OIL_TEMP,         "Temp. oleju",                "Oil temp.")                                  \
  X(S_OIL_PRESSURE,     "Cisnienie oleju",            "Oil pressure")                               \
  X(S_PEDALS_TITLE,     "PEDALY",                     "PEDALS")                                     \
  X(S_TRUCK_TITLE,      "CIEZAROWKA",                 "TRUCK")                                      \
  X(S_CRUISE,           "Tempomat",                   "Cruise")                                     \
  X(S_LIMIT,            "Limit",                      "Limit")                                      \
  X(S_AIR,              "Powietrze",                  "Air")                                        \
  X(S_ADBLUE,           "AdBlue",                     "AdBlue")                                     \
  X(S_RETARDER,         "Retarder",                   "Retarder")                                   \
  X(S_RANGE,            "Zasieg",                     "Range")                                      \
  X(S_NAV_DISTANCE,     "Do celu",                    "To target")                                  \
  X(S_NAV_TIME,         "Czas",                       "Time")                                       \
  X(S_OFF,              "wyl.",                       "off")                                        \
  X(S_NONE,             "brak",                       "none")                                       \
  X(S_LAMP_HANDBRAKE,  "HB",                         "HB")                                         \
  X(S_LAMP_LOW_BEAM,    "MIJ",                        "LOW")                                        \
  X(S_LAMP_HIGH_BEAM,   "DLUG",                       "HIGH")                                       \
  X(S_LAMP_ENGINE_BRAKE, "HAM.S",                     "E.BRK")                                      \
  X(S_LAMP_HAZARD,      "AWAR",                       "HAZ")

#define LANG_ENUM_ID(id, ...) id,
enum StrId : uint16_t { LANG_STRINGS(LANG_ENUM_ID) STR_COUNT };
#undef LANG_ENUM_ID

// Tekst w bieżącym języku
const char* tr(StrId id);

namespace lang {
void begin();          // wczytuje język z NVS (wywołać przed rysowaniem czegokolwiek)
Lang current();
void next();           // następny język z listy, od razu zapisany w NVS
void clear();          // kasuje zapisany język (reset urządzenia)
}  // namespace lang
