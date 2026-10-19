// Odczyt przycisków fizycznych (piny w config.h) i wysyłanie zdarzeń do Dashboard.
// Zdarzenie idzie raz, w chwili wciśnięcia (z eliminacją drgań styków).
// Wyjątek: przycisk 4 rozróżnia krótkie i długie wciśnięcie:
//  - puszczony przed BTN4_HOLD_MS -> PutButton4() (przy puszczeniu),
//  - trzymany BTN4_HOLD_MS        -> PutButton4Long() (kasowanie WiFi).
#pragma once

#include <Arduino.h>

class Dashboard;

class InReceiver {
public:
  explicit InReceiver(Dashboard& dashboard);

  void begin();
  void loop();   // wywoływać w loop()

private:
  static const uint8_t BUTTON_COUNT = 4;
  static const uint32_t DEBOUNCE_MS = 30;

  struct Button {
    uint8_t pin;
    uint32_t holdMs;       // 0 = zdarzenie od razu, inaczej krótkie/długie wciśnięcie
    bool pressed;          // stan po eliminacji drgań
    bool lastRaw;          // ostatni odczyt z pinu
    uint32_t changedMs;    // kiedy lastRaw się zmienił
    uint32_t pressedMs;    // kiedy pressed zmienił się na true
    bool sent;             // zdarzenie dla tego wciśnięcia już wysłane
  };

  void sendPressed(uint8_t index);
  void sendLong(uint8_t index);

  Dashboard& _dashboard;
  Button _buttons[BUTTON_COUNT];
};
