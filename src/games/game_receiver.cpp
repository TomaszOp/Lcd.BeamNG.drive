#include "game_receiver.h"

GameReceiver::GameReceiver(uint16_t port, TelemetryListener& listener, uint32_t timeoutMs)
    : _port(port), _listener(listener), _timeoutMs(timeoutMs) {}

bool GameReceiver::begin() {
  bool ok = _udp.begin(_port);
  Serial.printf("[UDP] Nasluch na porcie %u: %s\n", _port, ok ? "OK" : "BLAD");
  return ok;
}

void GameReceiver::loop() {
  // Odczytaj wszystko, co czeka w buforze, i przekaż tylko najnowszy pakiet –
  // przy chwilowym przycięciu nie wyświetlamy starych danych.
  bool gotPacket = false;
  uint8_t buf[256];
  while (_udp.parsePacket() > 0) {
    int len = _udp.read(buf, sizeof(buf));
    if (len <= 0) continue;

    const GameDef* game = nullptr;
    for (int i = 0; i < GAME_COUNT && !game; i++)
      if (GAMES[i].parse(buf, len, _data)) game = &GAMES[i];

    if (!game) continue;   // pakiet nieznany albo gra w pauzie
    sanitize(_data);
    if (game != _game) {
      _game = game;
      Serial.printf("[Gra] %s\n", game->name);
    }
    gotPacket = true;
  }

  uint32_t now = millis();
  if (gotPacket) {
    _lastPacketMs = now;
    _receiving = true;
    _listener.onTelemetry(*_game, _data);
  } else if (_receiving && now - _lastPacketMs > _timeoutMs) {
    _receiving = false;
    _listener.onTelemetryLost();
  }
}
