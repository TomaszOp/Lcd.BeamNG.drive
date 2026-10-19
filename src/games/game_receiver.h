// Gniazdo UDP: odbiera pakiety wszystkich gier z listy GAMES (games/game_list.cpp),
// rozpoznaje grę i przekazuje wspólne CarData do TelemetryListener.
#pragma once

#include <WiFiUdp.h>

#include "game_def.h"

// Odbiorca zdarzeń z GameReceiver.
class TelemetryListener {
public:
  virtual ~TelemetryListener() = default;
  // Przyszły nowe dane z gry.
  virtual void onTelemetry(const GameDef& game, const CarData& data) = 0;
  // Dane przestały przychodzić (pauza, menu, gra zamknięta).
  virtual void onTelemetryLost() = 0;
};

class GameReceiver {
public:
  GameReceiver(uint16_t port, TelemetryListener& listener, uint32_t timeoutMs = OUTGAUGE_TIMEOUT_MS);

  bool begin();   // otwiera gniazdo – wywołać po połączeniu z WiFi
  void loop();    // wywoływać w loop()

  bool hasData() const { return _receiving; }
  uint16_t port() const { return _port; }

private:
  uint16_t _port;
  TelemetryListener& _listener;
  uint32_t _timeoutMs;
  WiFiUDP _udp;
  CarData _data;
  const GameDef* _game = nullptr;
  uint32_t _lastPacketMs = 0;
  uint32_t _lastUnknownLogMs = 0;
  bool _receiving = false;
};
