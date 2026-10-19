// Ekran konfiguracji: lista opcji obsługiwana dotykiem albo przyciskami.
//   przycisk 1 = uruchom podświetloną opcję, 2 = poprzednia, 3 = następna, 4 = wyjście.
// Opcje: Konfiguracja WiFi (z potwierdzeniem), Kalibracja dotyku, Poziom jasnosci,
// Jezyk (PL/EN, od razu), Reset urzadzenia (z potwierdzeniem).
// Nowa opcja = wpis w ITEMS (config_screen.cpp) + obsługa w runItem().
#pragma once

#include <TFT_eSPI.h>

#include "../backlight.h"
#include "../lang.h"
#include "../touch.h"

class ConfigScreen {
public:
  // Co ma zrobić Dashboard po obsłużeniu zdarzenia.
  enum class Result { Stay, Close, ResetWifi, FactoryReset };

  ConfigScreen(TFT_eSPI& tft, Touch& touch, Backlight& backlight)
      : _tft(tft), _touch(touch), _backlight(backlight) {}

  void enter();                       // otwiera menu (lista, pierwsza opcja podświetlona)
  Result button(uint8_t n);           // przycisk fizyczny 1..4
  Result tap(uint16_t x, uint16_t y); // dotknięcie ekranu

private:
  enum class Page { List, ConfirmWifi, ConfirmFactory, Brightness };

  Result runItem(uint8_t index);
  Result confirm(bool yes);           // odpowiedź na stronie ConfirmWifi / ConfirmFactory
  Result closeBrightness(bool save);
  void changeBrightness(int delta);

  void drawList();
  void drawItem(uint8_t index);
  void drawConfirm(StrId title, StrId ask1, StrId ask2);
  void drawConfirmButtons();
  void drawBrightness();
  void drawBrightnessValue();
  void drawHeader(const char* title, const char* hint);
  void drawButton(int x, int y, int w, int h, const char* label, bool selected);

  TFT_eSPI& _tft;
  Touch& _touch;
  Backlight& _backlight;

  Page _page = Page::List;
  uint8_t _selected = 0;       // podświetlona opcja na liście
  bool _confirmYes = false;    // podświetlenie w potwierdzeniu (domyślnie "Nie")
  uint8_t _savedLevel = 100;   // jasność sprzed zmian (anulowanie)
};
