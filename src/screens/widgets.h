// Elementy wspólne dla ekranów: kolory, kontrolki, paski, bieg.
#pragma once

#include <TFT_eSPI.h>

namespace ui {

const uint16_t DIM = 0x2104;          // ciemnoszary – tło pasków, wygaszone elementy
const uint16_t LABEL = TFT_DARKGREY;  // opisy
const uint16_t YELLOW = 0xFE60;

// Rząd kontrolek: < ABS TC HB HI >
int lightsWidth();
uint32_t lightsMask();                // bity L_* pokazywane w rzędzie
void drawLights(TFT_eSPI& gfx, int x, int y, uint32_t lights);

// Pojedyncza lampka z napisem
void drawLamp(TFT_eSPI& gfx, int x, int y, int w, const char* label, bool on, uint16_t color);

// Paski: filled = wypełnienie w pikselach. Wypełnienie i tło rysowane rozłącznie – bez migania.
void drawHBar(TFT_eSPI& gfx, int x, int y, int w, int h, int filled, uint16_t color);
void drawVBar(TFT_eSPI& gfx, int x, int y, int w, int h, int filled, uint16_t color);   // od dołu

// Ikona konfiguracji (zębatka) w lewym dolnym rogu ekranów auta – dotknięcie otwiera menu
const int CONFIG_ICON_X = 16, CONFIG_ICON_Y = 222;   // środek ikony
void drawConfigIcon(TFT_eSPI& gfx);
bool configIconHit(uint16_t x, uint16_t y);          // strefa dotyku, większa niż sama ikona

// Bieg z CarData (>0 do przodu, 0 = N, -1 = R, -2 = R2...)
String gearText(int8_t gear);
uint16_t gearColor(int8_t gear);

// Wartość z jednostką, NAN (gra nie podaje) -> "--"
String valueText(float value, int decimals, const char* unit);

}  // namespace ui
