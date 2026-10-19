// Wyświetlanie danych z gry na LCD i sterowanie diodą RGB.
// Odbiera zdarzenia od GameReceiver (TelemetryListener) i InReceiver (przyciski).
// Rysowaniem kokpitu zajmuje się aktywny ekran (screens/), Dashboard wybiera ekran
// z listy ekranów bieżącej gry (GameDef::screens, games/game_list.cpp),
// pilnuje liczby klatek i pokazuje ekrany informacyjne (WiFi, czekanie na dane).
// Ekran konfiguracji: przycisk 2 albo dotknięcie ikony w lewym dolnym rogu ekranu auta;
// gdy jest otwarty, przyciski i dotyk idą do ConfigScreen.
// Dotyk środka lewej/prawej krawędzi ekranu auta = poprzedni/następny ekran.
#pragma once

#include <TFT_eSPI.h>

#include "backlight.h"
#include "games/game_receiver.h"
#include "rgb_led.h"
#include "screens/config_screen.h"
#include "screens/engine_screen.h"
#include "screens/gauges_screen.h"
#include "screens/pedals_screen.h"
#include "screens/race_screen.h"
#include "screens/truck_screen.h"
#include "touch.h"

class Dashboard : public TelemetryListener {
public:
  Dashboard(TFT_eSPI& tft, RgbLed& led, Touch& touch, Backlight& backlight);

  void begin();
  void loop();

  // TelemetryListener
  void onTelemetry(const GameDef& game, const CarData& data) override;
  void onTelemetryLost() override;

  // Zdarzenia z InReceiver – wciśnięcie przycisku fizycznego
  // W ekranie konfiguracji: 1 = OK, 2 = poprzednia opcja, 3 = następna, 4 = wyjście.
  void PutButton1();       // następny ekran gry (po ostatnim wraca do pierwszego)
  void PutButton2();       // otwiera ekran konfiguracji
  void PutButton3();
  void PutButton4();       // krótkie wciśnięcie – poza konfiguracją nic
  void PutButton4Long();   // przytrzymanie – kasuje dane WiFi i restartuje do hotspotu

  // Ekrany informacyjne
  void showConnecting(const String& ssid);
  void showPortal(const String& apSsid, const String& apPassword, const String& ip);
  void showWaiting(const String& ip, uint16_t port);

  // Wgrywanie OTA (wywołuje Ota)
  void showOtaStart();
  void showOtaProgress(uint8_t percent);
  void showOtaError();

private:
  enum class Info { None, Portal, Waiting };   // ostatni ekran informacyjny

  void messageScreen(const char* title);
  Screen* screen(ScreenId id);
  Screen* currentScreen();
  void enterScreen();
  void switchScreen(int step);   // 1 = następny, -1 = poprzedni
  void updateLed(const Telemetry& t);

  void pollTouch(uint32_t now);
  void openConfig();
  void closeConfig();
  void handleConfig(ConfigScreen::Result r);
  void resetWifi();
  void factoryReset();

  TFT_eSPI& _tft;
  RgbLed& _led;
  Touch& _touch;

  RaceScreen _race;
  GaugesScreen _gauges;
  EngineScreen _engine;
  PedalsScreen _pedals;
  TruckScreen _truck;
  ConfigScreen _config;
  bool _configOpen = false;
  bool _touchDown = false;      // palec na ekranie w poprzednim odczycie (reagujemy na dotknięcie)
  uint32_t _lastTouchMs = 0;
  const GameDef* _game = &GAMES[0];   // bieżąca gra (do pierwszych danych – pierwsza z listy)
  uint8_t _current = 0;         // indeks w _game->screens; po starcie zawsze ekran 1
  bool _screenActive = false;   // false = pokazany ekran informacyjny

  CarData _data;
  bool _hasData = false;
  bool _hasGame = false;        // przyszły już dane z jakiejś gry (nazwa na ekranie "Czekam")
  float _maxRpm;
  float _topSpeed = 0;          // km/h, zerowana razem z _maxRpm (skala prędkościomierza)
  uint32_t _carKey = 0;         // odcisk auta z parsera gry (CarData::carKey)
  uint32_t _lastFrameMs = 0;

  Info _info = Info::None;
  String _waitIp;
  uint16_t _waitPort = 0;
  String _apSsid, _apPassword, _apIp;
};
