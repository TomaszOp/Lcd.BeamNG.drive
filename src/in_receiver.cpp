#include "in_receiver.h"

#include "dashboard.h"

InReceiver::InReceiver(Dashboard& dashboard)
    : _dashboard(dashboard),
      _buttons{{PIN_BTN_1, 0, false, false, 0, 0, false},
               {PIN_BTN_2, 0, false, false, 0, 0, false},
               {PIN_BTN_3, 0, false, false, 0, 0, false},
               {PIN_BTN_4, BTN4_HOLD_MS, false, false, 0, 0, false}} {}

void InReceiver::begin() {
  for (Button& b : _buttons) {
    pinMode(b.pin, BTN_ACTIVE_LEVEL == LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
    b.lastRaw = digitalRead(b.pin) == BTN_ACTIVE_LEVEL;
    b.pressed = b.lastRaw;   // przycisk trzymany przy starcie nie wysyła zdarzenia
    b.sent = b.pressed;
    b.changedMs = b.pressedMs = millis();
  }
}

void InReceiver::loop() {
  uint32_t now = millis();
  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    Button& b = _buttons[i];
    bool raw = digitalRead(b.pin) == BTN_ACTIVE_LEVEL;

    if (raw != b.lastRaw) {
      b.lastRaw = raw;
      b.changedMs = now;
    } else if (raw != b.pressed && now - b.changedMs >= DEBOUNCE_MS) {
      b.pressed = raw;
      if (b.pressed) {
        b.pressedMs = now;
        b.sent = b.holdMs == 0;
        if (b.sent) sendPressed(i);          // zwykły przycisk – od razu
      } else if (!b.sent) {
        b.sent = true;
        sendPressed(i);                      // puszczony przed czasem = krótkie wciśnięcie
      }
    }

    if (b.pressed && !b.sent && now - b.pressedMs >= b.holdMs) {
      b.sent = true;
      sendLong(i);                           // trzymany wystarczająco długo
    }
  }
}

void InReceiver::sendPressed(uint8_t index) {
  switch (index) {
    case 0: _dashboard.PutButton1(); break;
    case 1: _dashboard.PutButton2(); break;
    case 2: _dashboard.PutButton3(); break;
    case 3: _dashboard.PutButton4(); break;
  }
}

void InReceiver::sendLong(uint8_t index) {
  if (index == 3) _dashboard.PutButton4Long();
}
