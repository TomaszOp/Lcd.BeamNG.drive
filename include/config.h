// =====================================================================
//  config.h – konfiguracja sprzętu
//
//  Piny są osobno dla każdej płytki – wybiera je flaga BOARD_* ustawiona
//  w build_flags środowiska w platformio.ini.
//
//  Plik jest dołączany automatycznie do całego projektu ORAZ do
//  biblioteki TFT_eSPI (patrz build_flags w platformio.ini), więc piny
//  wyświetlacza ustawia się tylko tutaj – User_Setup.h nie jest używany.
//
//  Uwaga: plik trafia też do plików C, więc wolno tu używać tylko #define.
// =====================================================================
#pragma once

// ---------------------------------------------------------------------
//  PINY – osobny zestaw dla każdej płytki
//
//  Dotyk XPT2046 jest na wspólnej szynie SPI z wyświetlaczem:
//    T_CLK -> TFT_SCLK, T_DIN -> TFT_MOSI, T_DO -> TFT_MISO,
//    T_CS -> TOUCH_CS, T_IRQ -> PIN_TOUCH_IRQ
//  TFT_MISO jest wymagany przez dotyk – nie ustawiać -1.
//  PIN_TOUCH_IRQ: LOW = dotyk; TFT_eSPI go nie używa, tylko nasz kod (-1 jeśli niepodłączony).
//  TFT_RST: -1 jeśli RST podpięty do EN/3V3.  TFT_BL: -1 jeśli podświetlenie na stałe do 3V3.
// ---------------------------------------------------------------------
#if defined(BOARD_ESP32S3)
// ESP32-S3 R8N16 – nie używać: 0, 3, 45, 46 (bootowanie), 19, 20 (USB),
// 26..37 (flash i PSRAM OPI)
  #define TFT_MOSI        11
  #define TFT_SCLK        12
  #define TFT_MISO        13
  #define TFT_CS          10
  #define TFT_DC           9
  #define TFT_RST         14
  #define TFT_BL          21

  #define TOUCH_CS         8
  #define PIN_TOUCH_IRQ   18

  #define PIN_BTN_1        4
  #define PIN_BTN_2        5
  #define PIN_BTN_3        6
  #define PIN_BTN_4        7

  #define PIN_LED_R       15
  #define PIN_LED_G       16
  #define PIN_LED_B       17

  // TFT_eSPI 2.5.43 + rdzen Arduino 3.x: FSPI ma wartosc 0, a biblioteka liczy z niej
  // adres rejestrow SPI -> pisze do SPI0 (flash) zamiast SPI2 i ekran zostaje pusty.
  // USE_FSPI_PORT ustawia SPI_PORT = 2, czyli wlasciwy sprzetowy FSPI (SPI2).
  #define USE_FSPI_PORT

#elif defined(BOARD_ESP32_WROOM)
// ESP32 WROOM-32 – SPI na pinach sprzętowego VSPI
  #define TFT_MOSI        23
  #define TFT_SCLK        18
  #define TFT_MISO        19
  #define TFT_CS           5
  #define TFT_DC          16
  #define TFT_RST         17
  #define TFT_BL           4

  #define TOUCH_CS        21
  #define PIN_TOUCH_IRQ   22

  #define PIN_BTN_1       32
  #define PIN_BTN_2       33
  #define PIN_BTN_3       25
  #define PIN_BTN_4       26

  #define PIN_LED_R       27
  #define PIN_LED_G       14
  #define PIN_LED_B       13

#else
  #error "Nieznana plytka: dodaj -D BOARD_... w platformio.ini i zestaw pinow w config.h"
#endif

// ---------------------------------------------------------------------
//  Wyświetlacz ILI9341 (SPI)
// ---------------------------------------------------------------------
#define USER_SETUP_LOADED 1     // TFT_eSPI: nie czytaj User_Setup.h
#define ILI9341_DRIVER

#define TFT_BACKLIGHT_ON HIGH

#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000

// Fonty wbudowane w TFT_eSPI
#define LOAD_GLCD               // 8 px
#define LOAD_FONT2              // 16 px
#define LOAD_FONT4              // 26 px
#define LOAD_FONT6              // 48 px, tylko cyfry
#define LOAD_FONT7              // 48 px, cyfry 7-segmentowe
#define LOAD_FONT8              // 75 px, tylko cyfry
#define LOAD_GFXFF              // FreeFonts
#define SMOOTH_FONT

// ---------------------------------------------------------------------
//  Dotyk XPT2046
// ---------------------------------------------------------------------
#define SPI_TOUCH_FREQUENCY 2500000

#define TOUCH_ENABLE 1
          // 0 = dotyk wylaczony (test samych przyciskow), 1 = wlaczony

#define TOUCH_DEBUG_LOG 0       // 1 = szczegolowe logi dotyku w Serial (surowe odczyty, rogi, test)

#define TOUCH_CORNER_TIMEOUT_MS 15000  // kalibracja: tyle czasu na dotknięcie rogu (brak = dotyk wyłączony)
#define TOUCH_TEST_TIME_MS       5000  // test: tyle czasu na dotknięcie każdego punktu
#define TOUCH_TEST_RADIUS          15  // test: trafienie, jeśli dotyk w tylu px od punktu
// Przełączanie ekranów auta dotykiem: lewy pas = poprzedni, prawy pas = następny (ekran 320x240)
#define TOUCH_NAV_WIDTH    80   // szerokość pasa przy lewej/prawej krawędzi
#define TOUCH_NAV_TOP      60   // pas od tej wysokości...
#define TOUCH_NAV_BOTTOM  180   // ...do tej (bez niej) – poniżej jest ikona konfiguracji

// ---------------------------------------------------------------------
//  Przyciski (do GND, wewnętrzny pull-up -> wciśnięty = LOW)
// ---------------------------------------------------------------------
#define BTN_ACTIVE_LEVEL LOW
#define BTN4_HOLD_MS 3000       // przycisk 4: trzymany tyle ms = kasowanie WiFi, krócej = zwykłe wciśnięcie

// ---------------------------------------------------------------------
//  Dioda RGB (osobne nogi R/G/B, sterowanie PWM)
// ---------------------------------------------------------------------
#define LED_COMMON_ANODE 0      // 1 = wspólna anoda (do 3V3), 0 = wspólna katoda (do GND)

// ---------------------------------------------------------------------
//  Sieć / OutGauge
// ---------------------------------------------------------------------
#define OUTGAUGE_PORT 4444      // taki sam jak w BeamNG: Options -> Other -> OutGauge
#define OUTGAUGE_TIMEOUT_MS 2000 // brak pakietów dłużej niż tyle = "brak danych"

#define WIFI_CONNECT_TIMEOUT_MS 15000 // po tym czasie bez połączenia -> tryb hotspot
#define WIFI_AP_SSID      "BeamNG-Dash"
#define WIFI_AP_PASSWORD  "beamng1234"  // min. 8 znaków, "" = sieć otwarta
#define WIFI_HOSTNAME     "beamng-dash"

// Wgrywanie przez WiFi (OTA): 1 = włączone, 0 = wyłączone (kod OTA się nie kompiluje).
// Wgrywanie: środowisko *_ota w platformio.ini (upload_flags --auth = OTA_PASSWORD).
#define OTA_ENABLE    1
#define OTA_HOSTNAME  WIFI_HOSTNAME   // unikalna nazwa w sieci: beamng-dash.local
#define OTA_PASSWORD  "BeamNG"
