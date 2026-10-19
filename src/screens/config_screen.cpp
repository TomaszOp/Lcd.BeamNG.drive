#include "config_screen.h"

#include "widgets.h"

// ---------------------------------------------------------------------
//  Układ ekranu 320x240
//
//   KONFIGURACJA                    [Wyjscie]   y 4..33
//   [        Konfiguracja WiFi           ]      y 40..
//   [        Kalibracja dotyku           ]      co 36 px
//   [        Poziom jasnosci             ]
//   [        Jezyk: polski               ]
//   [        Reset urzadzenia            ]      ..215
//   1 OK  2 w gore  3 w dol  4 wyjscie          y 222..237
// ---------------------------------------------------------------------

static const StrId ITEMS[] = {S_WIFI_SETUP, S_TOUCH_CALIBRATION, S_BRIGHTNESS_ITEM, S_LANGUAGE_ITEM,
                              S_FACTORY_RESET};
static const uint8_t ITEM_COUNT = sizeof(ITEMS) / sizeof(ITEMS[0]);
enum : uint8_t { ITEM_WIFI, ITEM_TOUCH, ITEM_BRIGHTNESS, ITEM_LANGUAGE, ITEM_FACTORY };

static const int ITEM_X = 20, ITEM_W = 280, ITEM_H = 32, ITEM_Y = 40, ITEM_STEP = 36;
static const int EXIT_X = 226, EXIT_Y = 4, EXIT_W = 90, EXIT_H = 30;   // dotykowy "Wyjscie"
static const int HINT_Y = 226;

// potwierdzenie (WiFi, reset urządzenia): [Tak] [Nie]
static const int YES_X = 30, NO_X = 170, CHOICE_Y = 140, CHOICE_W = 120, CHOICE_H = 50;

// jasność: [-]  70%  [+]   [Zapisz]
static const int MINUS_X = 20, PLUS_X = 240, ADJ_Y = 70, ADJ_W = 60, ADJ_H = 60;
static const int LEVEL_BAR_X = 20, LEVEL_BAR_Y = 146, LEVEL_BAR_W = 280, LEVEL_BAR_H = 14;
static const int SAVE_X = 90, SAVE_Y = 172, SAVE_W = 140, SAVE_H = 40;

static bool inside(uint16_t x, uint16_t y, int bx, int by, int bw, int bh) {
  return x >= bx && x < bx + bw && y >= by && y < by + bh;
}

// ---------------------------------------------------------------------

void ConfigScreen::enter() {
  _page = Page::List;
  _selected = 0;
  drawList();
}

ConfigScreen::Result ConfigScreen::button(uint8_t n) {
  switch (_page) {
    case Page::List:
      if (n == 1) return runItem(_selected);
      if (n == 2 || n == 3) {
        uint8_t old = _selected;
        _selected = (_selected + (n == 2 ? ITEM_COUNT - 1 : 1)) % ITEM_COUNT;
        drawItem(old);
        drawItem(_selected);
        return Result::Stay;
      }
      return Result::Close;   // 4

    case Page::ConfirmWifi:
    case Page::ConfirmFactory:
      if (n == 1) return confirm(_confirmYes);
      if (n == 2 || n == 3) {
        _confirmYes = !_confirmYes;
        drawConfirmButtons();
        return Result::Stay;
      }
      return confirm(false);   // 4 = wstecz

    case Page::Brightness:
      if (n == 1) return closeBrightness(true);
      if (n == 2) changeBrightness(-Backlight::STEP);
      if (n == 3) changeBrightness(Backlight::STEP);
      if (n == 4) return closeBrightness(false);
      return Result::Stay;
  }
  return Result::Stay;
}

ConfigScreen::Result ConfigScreen::tap(uint16_t x, uint16_t y) {
  switch (_page) {
    case Page::List:
      if (inside(x, y, EXIT_X, EXIT_Y, EXIT_W, EXIT_H)) return Result::Close;
      for (uint8_t i = 0; i < ITEM_COUNT; i++) {
        if (inside(x, y, ITEM_X, ITEM_Y + i * ITEM_STEP, ITEM_W, ITEM_H)) {
          uint8_t old = _selected;
          _selected = i;
          drawItem(old);
          drawItem(i);
          return runItem(i);
        }
      }
      return Result::Stay;

    case Page::ConfirmWifi:
    case Page::ConfirmFactory:
      if (inside(x, y, YES_X, CHOICE_Y, CHOICE_W, CHOICE_H)) return confirm(true);
      if (inside(x, y, NO_X, CHOICE_Y, CHOICE_W, CHOICE_H)) return confirm(false);
      return Result::Stay;

    case Page::Brightness:
      if (inside(x, y, MINUS_X, ADJ_Y, ADJ_W, ADJ_H)) changeBrightness(-Backlight::STEP);
      else if (inside(x, y, PLUS_X, ADJ_Y, ADJ_W, ADJ_H)) changeBrightness(Backlight::STEP);
      else if (inside(x, y, SAVE_X, SAVE_Y, SAVE_W, SAVE_H)) return closeBrightness(true);
      else if (inside(x, y, EXIT_X, EXIT_Y, EXIT_W, EXIT_H)) return closeBrightness(false);
      return Result::Stay;
  }
  return Result::Stay;
}

// ---------------------------------------------------------------------
//  Opcje

ConfigScreen::Result ConfigScreen::runItem(uint8_t index) {
  switch (index) {
    case ITEM_WIFI:
      _page = Page::ConfirmWifi;
      _confirmYes = false;
      drawConfirm(S_WIFI_TITLE, S_WIFI_ASK1, S_WIFI_ASK2);
      break;

    case ITEM_TOUCH:
      _touch.recalibrate();   // blokuje na czas kalibracji, rysuje własne ekrany
      drawList();
      break;

    case ITEM_BRIGHTNESS:
      _page = Page::Brightness;
      _savedLevel = _backlight.level();
      drawBrightness();
      break;

    case ITEM_LANGUAGE:
      lang::next();   // zapis w NVS; pozostałe ekrany rysują teksty przy wejściu
      drawList();
      break;

    case ITEM_FACTORY:
      _page = Page::ConfirmFactory;
      _confirmYes = false;
      drawConfirm(S_FACTORY_TITLE, S_FACTORY_ASK1, S_FACTORY_ASK2);
      break;
  }
  return Result::Stay;
}

ConfigScreen::Result ConfigScreen::confirm(bool yes) {
  if (yes) return _page == Page::ConfirmWifi ? Result::ResetWifi : Result::FactoryReset;
  _page = Page::List;
  drawList();
  return Result::Stay;
}

void ConfigScreen::changeBrightness(int delta) {
  _backlight.set(constrain(_backlight.level() + delta, (int)Backlight::MIN_LEVEL, 100));
  drawBrightnessValue();
}

ConfigScreen::Result ConfigScreen::closeBrightness(bool save) {
  if (save) _backlight.save();
  else _backlight.set(_savedLevel);
  _page = Page::List;
  drawList();
  return Result::Stay;
}

// ---------------------------------------------------------------------
//  Rysowanie

void ConfigScreen::drawHeader(const char* title, const char* hint) {
  _tft.fillScreen(TFT_BLACK);
  _tft.setTextPadding(0);
  _tft.setTextDatum(TL_DATUM);
  _tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  _tft.drawString(title, 6, 8, 4);

  _tft.setTextDatum(BC_DATUM);
  _tft.setTextColor(ui::LABEL, TFT_BLACK);
  _tft.drawString(hint, 160, 238, 2);
}

void ConfigScreen::drawButton(int x, int y, int w, int h, const char* label, bool selected) {
  uint16_t bg = selected ? TFT_ORANGE : TFT_BLACK;
  _tft.fillRoundRect(x, y, w, h, 6, bg);
  _tft.drawRoundRect(x, y, w, h, 6, selected ? TFT_ORANGE : ui::LABEL);
  _tft.setTextDatum(MC_DATUM);
  _tft.setTextPadding(0);
  _tft.setTextColor(selected ? TFT_BLACK : TFT_WHITE, bg);
  _tft.drawString(label, x + w / 2, y + h / 2 + 1, 4);
}

void ConfigScreen::drawList() {
  drawHeader(tr(S_CONFIG_TITLE), tr(S_CONFIG_HINT));
  drawButton(EXIT_X, EXIT_Y, EXIT_W, EXIT_H, "", false);
  _tft.setTextDatum(MC_DATUM);
  _tft.setTextColor(TFT_WHITE, TFT_BLACK);
  _tft.drawString(tr(S_EXIT), EXIT_X + EXIT_W / 2, EXIT_Y + EXIT_H / 2 + 1, 2);
  for (uint8_t i = 0; i < ITEM_COUNT; i++) drawItem(i);
}

void ConfigScreen::drawItem(uint8_t index) {
  drawButton(ITEM_X, ITEM_Y + index * ITEM_STEP, ITEM_W, ITEM_H, tr(ITEMS[index]), index == _selected);
}

void ConfigScreen::drawConfirm(StrId title, StrId ask1, StrId ask2) {
  drawHeader(tr(title), tr(S_CONFIRM_HINT));
  _tft.setTextDatum(TC_DATUM);
  _tft.setTextColor(TFT_WHITE, TFT_BLACK);
  _tft.drawString(tr(ask1), 160, 60, 2);
  _tft.drawString(tr(ask2), 160, 80, 2);
  _tft.setTextColor(ui::LABEL, TFT_BLACK);
  _tft.drawString(tr(S_WILL_RESTART), 160, 106, 2);
  drawConfirmButtons();
}

void ConfigScreen::drawConfirmButtons() {
  drawButton(YES_X, CHOICE_Y, CHOICE_W, CHOICE_H, tr(S_YES), _confirmYes);
  drawButton(NO_X, CHOICE_Y, CHOICE_W, CHOICE_H, tr(S_NO), !_confirmYes);
}

void ConfigScreen::drawBrightness() {
  drawHeader(tr(S_BRIGHTNESS_TITLE), tr(S_BRIGHTNESS_HINT));
  drawButton(EXIT_X, EXIT_Y, EXIT_W, EXIT_H, "", false);
  _tft.setTextDatum(MC_DATUM);
  _tft.setTextColor(TFT_WHITE, TFT_BLACK);
  _tft.drawString(tr(S_CANCEL), EXIT_X + EXIT_W / 2, EXIT_Y + EXIT_H / 2 + 1, 2);
  drawButton(MINUS_X, ADJ_Y, ADJ_W, ADJ_H, "-", false);
  drawButton(PLUS_X, ADJ_Y, ADJ_W, ADJ_H, "+", false);
  drawButton(SAVE_X, SAVE_Y, SAVE_W, SAVE_H, tr(S_SAVE), true);
  drawBrightnessValue();
}

void ConfigScreen::drawBrightnessValue() {
  uint8_t level = _backlight.level();
  _tft.setTextDatum(MC_DATUM);
  _tft.setTextColor(TFT_WHITE, TFT_BLACK);
  _tft.setTextPadding(_tft.textWidth("100%", 4) + 10);
  _tft.drawString(String(level) + "%", 160, ADJ_Y + ADJ_H / 2, 4);
  _tft.setTextPadding(0);
  ui::drawHBar(_tft, LEVEL_BAR_X, LEVEL_BAR_Y, LEVEL_BAR_W, LEVEL_BAR_H, level * LEVEL_BAR_W / 100, TFT_ORANGE);
}
