// Wtyczka telemetrii ETS2 / ATS (SCS Telemetry SDK) dla wyświetlacza BeamNG Dash.
// Zbiera dane ciężarówki z kanałów SDK i co 1/rate s wysyła je jako TruckPacket
// po UDP na adres z pliku beamdash.ini (leży obok wtyczki).
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <chrono>
#include <string>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  #define PLUGIN_EXPORT extern "C" __declspec(dllexport)
  typedef SOCKET socket_t;
  static const socket_t NO_SOCKET = INVALID_SOCKET;
  static void closeSocket(socket_t s) { closesocket(s); }
#else
  #include <arpa/inet.h>
  #include <dlfcn.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
  #define PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
  typedef int socket_t;
  static const socket_t NO_SOCKET = -1;
  static void closeSocket(socket_t s) { close(s); }
#endif

#include "scssdk_telemetry.h"
#include "eurotrucks2/scssdk_eut2.h"
#include "eurotrucks2/scssdk_telemetry_eut2.h"
#include "amtrucks/scssdk_ats.h"
#include "amtrucks/scssdk_telemetry_ats.h"
#include "common/scssdk_telemetry_common_configs.h"
#include "common/scssdk_telemetry_truck_common_channels.h"

#include "truck_packet.h"

// ---------------------------------------------------------------------
//  Stan wtyczki

struct Settings {
  std::string ip = "255.255.255.255";   // domyślnie broadcast w sieci lokalnej
  int port = 4444;                      // ten sam port co OutGauge z BeamNG
  int rate = 30;                        // pakietów na sekundę
};

static Settings g_settings;
static scs_log_t g_log = nullptr;
static socket_t g_socket = NO_SOCKET;
static sockaddr_in g_target{};
static TruckPacket g_packet{};
static std::chrono::steady_clock::time_point g_lastSend;

static void log(scs_log_type_t type, const char* fmt, ...) {
  if (!g_log) return;
  char text[256] = "[BeamDash] ";
  va_list args;
  va_start(args, fmt);
  vsnprintf(text + 11, sizeof(text) - 11, fmt, args);
  va_end(args);
  g_log(type, text);
}

// ---------------------------------------------------------------------
//  Plik konfiguracji beamdash.ini (ip=, port=, rate=) obok pliku wtyczki

static std::string pluginDir() {
  char path[1024] = "";
#ifdef _WIN32
  HMODULE module = nullptr;
  GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                     reinterpret_cast<LPCSTR>(&pluginDir), &module);
  GetModuleFileNameA(module, path, sizeof(path));
#else
  Dl_info info;
  if (dladdr(reinterpret_cast<void*>(&pluginDir), &info) && info.dli_fname)
    snprintf(path, sizeof(path), "%s", info.dli_fname);
#endif
  std::string dir = path;
  size_t slash = dir.find_last_of("/\\");
  return slash == std::string::npos ? std::string() : dir.substr(0, slash + 1);
}

static std::string trim(const std::string& s) {
  size_t b = s.find_first_not_of(" \t\r\n"), e = s.find_last_not_of(" \t\r\n");
  return b == std::string::npos ? std::string() : s.substr(b, e - b + 1);
}

static void loadSettings() {
  g_settings = Settings();
  std::string file = pluginDir() + "beamdash.ini";
  FILE* f = fopen(file.c_str(), "r");
  if (!f) {
    log(SCS_LOG_TYPE_warning, "brak %s - domyslnie %s:%d", file.c_str(), g_settings.ip.c_str(), g_settings.port);
    return;
  }
  char line[256];
  while (fgets(line, sizeof(line), f)) {
    std::string s = trim(line);
    if (s.empty() || s[0] == '#' || s[0] == ';' || s[0] == '[') continue;
    size_t eq = s.find('=');
    if (eq == std::string::npos) continue;
    std::string key = trim(s.substr(0, eq)), value = trim(s.substr(eq + 1));
    if (key == "ip") g_settings.ip = value;
    else if (key == "port") g_settings.port = atoi(value.c_str());
    else if (key == "rate") g_settings.rate = atoi(value.c_str());
  }
  fclose(f);
  if (g_settings.rate < 1) g_settings.rate = 1;
  if (g_settings.rate > 100) g_settings.rate = 100;
}

// ---------------------------------------------------------------------
//  UDP

static bool openSocket() {
#ifdef _WIN32
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
#endif
  g_target = sockaddr_in{};
  g_target.sin_family = AF_INET;
  g_target.sin_port = htons((uint16_t)g_settings.port);
  if (inet_pton(AF_INET, g_settings.ip.c_str(), &g_target.sin_addr) != 1) {
    log(SCS_LOG_TYPE_error, "zly adres ip=%s w beamdash.ini", g_settings.ip.c_str());
    return false;
  }
  g_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (g_socket == NO_SOCKET) return false;
  int yes = 1;   // pozwala wysyłać na adres broadcast
  setsockopt(g_socket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&yes), sizeof(yes));
  return true;
}

static void closeUdp() {
  if (g_socket != NO_SOCKET) closeSocket(g_socket);
  g_socket = NO_SOCKET;
#ifdef _WIN32
  WSACleanup();
#endif
}

static void sendPacket() {
  if (g_socket == NO_SOCKET) return;
  g_packet.counter++;
  sendto(g_socket, reinterpret_cast<const char*>(&g_packet), sizeof(g_packet), 0,
         reinterpret_cast<const sockaddr*>(&g_target), sizeof(g_target));
}

// ---------------------------------------------------------------------
//  Kanały – callbacki zapisują wartość prosto do g_packet

static void setFlag(uint32_t bit, bool on) {
  if (on) g_packet.flags |= bit;
  else g_packet.flags &= ~bit;
}

SCSAPI_VOID onFloat(const scs_string_t, const scs_u32_t, const scs_value_t* const value, const scs_context_t context) {
  *static_cast<float*>(context) = value ? value->value_float.value : 0.0f;
}

SCSAPI_VOID onBool(const scs_string_t, const scs_u32_t, const scs_value_t* const value, const scs_context_t context) {
  setFlag((uint32_t)(uintptr_t)context, value && value->value_bool.value);
}

SCSAPI_VOID onGear(const scs_string_t, const scs_u32_t, const scs_value_t* const value, const scs_context_t) {
  int32_t gear = value ? value->value_s32.value : 0;
  g_packet.gear = (int8_t)(gear > 127 ? 127 : gear < -127 ? -127 : gear);
}

SCSAPI_VOID onRetarder(const scs_string_t, const scs_u32_t, const scs_value_t* const value, const scs_context_t) {
  uint32_t level = value ? value->value_u32.value : 0;
  g_packet.retarder = (uint8_t)(level > 255 ? 255 : level);
}

// Kanały zapisują float przez wskaźnik do pola spakowanej struktury – bezpieczne,
// bo wszystkie pola float leżą na adresach podzielnych przez 4 (sprawdzane niżej).
static_assert(offsetof(TruckPacket, speed) % 4 == 0 && offsetof(TruckPacket, fuel) % 4 == 0 &&
              offsetof(TruckPacket, navTime) % 4 == 0, "pola float TruckPacket musza byc wyrownane do 4");
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"

struct FloatChannel { const char* name; float* target; };
struct BoolChannel { const char* name; uint32_t bit; };

static void registerChannels(const scs_telemetry_init_params_v100_t* p) {
  const FloatChannel floats[] = {
      {SCS_TELEMETRY_TRUCK_CHANNEL_speed, &g_packet.speed},
      {SCS_TELEMETRY_TRUCK_CHANNEL_engine_rpm, &g_packet.rpm},
      {SCS_TELEMETRY_TRUCK_CHANNEL_cruise_control, &g_packet.cruiseSpeed},
      {SCS_TELEMETRY_TRUCK_CHANNEL_navigation_speed_limit, &g_packet.speedLimit},
      {SCS_TELEMETRY_TRUCK_CHANNEL_fuel, &g_packet.fuel},
      {SCS_TELEMETRY_TRUCK_CHANNEL_fuel_range, &g_packet.fuelRange},
      {SCS_TELEMETRY_TRUCK_CHANNEL_adblue, &g_packet.adblue},
      {SCS_TELEMETRY_TRUCK_CHANNEL_brake_air_pressure, &g_packet.airPressure},
      {SCS_TELEMETRY_TRUCK_CHANNEL_brake_temperature, &g_packet.brakeTemp},
      {SCS_TELEMETRY_TRUCK_CHANNEL_oil_temperature, &g_packet.oilTemp},
      {SCS_TELEMETRY_TRUCK_CHANNEL_oil_pressure, &g_packet.oilPressure},
      {SCS_TELEMETRY_TRUCK_CHANNEL_water_temperature, &g_packet.waterTemp},
      {SCS_TELEMETRY_TRUCK_CHANNEL_battery_voltage, &g_packet.batteryVoltage},
      {SCS_TELEMETRY_TRUCK_CHANNEL_effective_throttle, &g_packet.throttle},
      {SCS_TELEMETRY_TRUCK_CHANNEL_effective_brake, &g_packet.brake},
      {SCS_TELEMETRY_TRUCK_CHANNEL_effective_clutch, &g_packet.clutch},
      {SCS_TELEMETRY_TRUCK_CHANNEL_odometer, &g_packet.odometer},
      {SCS_TELEMETRY_TRUCK_CHANNEL_navigation_distance, &g_packet.navDistance},
      {SCS_TELEMETRY_TRUCK_CHANNEL_navigation_time, &g_packet.navTime},
  };
  const BoolChannel bools[] = {
      {SCS_TELEMETRY_TRUCK_CHANNEL_engine_enabled, TF_ENGINE},
      {SCS_TELEMETRY_TRUCK_CHANNEL_electric_enabled, TF_ELECTRIC},
      {SCS_TELEMETRY_TRUCK_CHANNEL_parking_brake, TF_PARKING_BRAKE},
      {SCS_TELEMETRY_TRUCK_CHANNEL_motor_brake, TF_MOTOR_BRAKE},
      {SCS_TELEMETRY_TRUCK_CHANNEL_differential_lock, TF_DIFF_LOCK},
      {SCS_TELEMETRY_TRUCK_CHANNEL_light_lblinker, TF_BLINKER_L},
      {SCS_TELEMETRY_TRUCK_CHANNEL_light_rblinker, TF_BLINKER_R},
      {SCS_TELEMETRY_TRUCK_CHANNEL_hazard_warning, TF_HAZARD},
      {SCS_TELEMETRY_TRUCK_CHANNEL_light_parking, TF_PARKING_LIGHTS},
      {SCS_TELEMETRY_TRUCK_CHANNEL_light_low_beam, TF_LOW_BEAM},
      {SCS_TELEMETRY_TRUCK_CHANNEL_light_high_beam, TF_HIGH_BEAM},
      {SCS_TELEMETRY_TRUCK_CHANNEL_light_beacon, TF_BEACON},
      {SCS_TELEMETRY_TRUCK_CHANNEL_brake_air_pressure_warning, TF_WARN_AIR},
      {SCS_TELEMETRY_TRUCK_CHANNEL_brake_air_pressure_emergency, TF_WARN_AIR_EMERG},
      {SCS_TELEMETRY_TRUCK_CHANNEL_fuel_warning, TF_WARN_FUEL},
      {SCS_TELEMETRY_TRUCK_CHANNEL_adblue_warning, TF_WARN_ADBLUE},
      {SCS_TELEMETRY_TRUCK_CHANNEL_oil_pressure_warning, TF_WARN_OIL},
      {SCS_TELEMETRY_TRUCK_CHANNEL_water_temperature_warning, TF_WARN_WATER},
      {SCS_TELEMETRY_TRUCK_CHANNEL_battery_voltage_warning, TF_WARN_BATTERY},
  };

  // brak kanału w starszej wersji gry nie jest błędem – pole zostaje 0
  for (const FloatChannel& c : floats)
    p->register_for_channel(c.name, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, onFloat, c.target);
  for (const BoolChannel& c : bools)
    p->register_for_channel(c.name, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, onBool,
                            reinterpret_cast<scs_context_t>((uintptr_t)c.bit));
  p->register_for_channel(SCS_TELEMETRY_TRUCK_CHANNEL_displayed_gear, SCS_U32_NIL, SCS_VALUE_TYPE_s32,
                          SCS_TELEMETRY_CHANNEL_FLAG_none, onGear, nullptr);
  p->register_for_channel(SCS_TELEMETRY_TRUCK_CHANNEL_retarder_level, SCS_U32_NIL, SCS_VALUE_TYPE_u32,
                          SCS_TELEMETRY_CHANNEL_FLAG_none, onRetarder, nullptr);
}

// ---------------------------------------------------------------------
//  Zdarzenia

static void copyString(char* dst, size_t size, const char* src) {
  snprintf(dst, size, "%s", src ? src : "");
}

// Konfiguracja ciężarówki: przychodzi po wejściu do gry i po każdej zmianie pojazdu.
static void onTruckConfig(const scs_named_value_t* attr) {
  g_packet.rpmMax = g_packet.fuelCapacity = g_packet.adblueCapacity = 0;
  g_packet.gearsForward = g_packet.gearsReverse = g_packet.retarderSteps = 0;
  g_packet.brand[0] = g_packet.name[0] = 0;

  bool any = false;
  for (; attr && attr->name; attr++) {
    const scs_value_t& v = attr->value;
    const char* n = attr->name;
    any = true;
    if (!strcmp(n, SCS_TELEMETRY_CONFIG_ATTRIBUTE_brand)) copyString(g_packet.brand, sizeof(g_packet.brand), v.value_string.value);
    else if (!strcmp(n, SCS_TELEMETRY_CONFIG_ATTRIBUTE_name)) copyString(g_packet.name, sizeof(g_packet.name), v.value_string.value);
    else if (!strcmp(n, SCS_TELEMETRY_CONFIG_ATTRIBUTE_rpm_limit)) g_packet.rpmMax = v.value_float.value;
    else if (!strcmp(n, SCS_TELEMETRY_CONFIG_ATTRIBUTE_fuel_capacity)) g_packet.fuelCapacity = v.value_float.value;
    else if (!strcmp(n, SCS_TELEMETRY_CONFIG_ATTRIBUTE_adblue_capacity)) g_packet.adblueCapacity = v.value_float.value;
    else if (!strcmp(n, SCS_TELEMETRY_CONFIG_ATTRIBUTE_forward_gear_count)) g_packet.gearsForward = (uint8_t)v.value_u32.value;
    else if (!strcmp(n, SCS_TELEMETRY_CONFIG_ATTRIBUTE_reverse_gear_count)) g_packet.gearsReverse = (uint8_t)v.value_u32.value;
    else if (!strcmp(n, SCS_TELEMETRY_CONFIG_ATTRIBUTE_retarder_step_count)) g_packet.retarderSteps = (uint8_t)v.value_u32.value;
  }
  setFlag(TF_TRUCK_VALID, any);   // pusta lista = brak ciężarówki
  log(SCS_LOG_TYPE_message, "ciezarowka: %s %s, max %.0f rpm, biegi %u/%u", g_packet.brand, g_packet.name,
      g_packet.rpmMax, g_packet.gearsForward, g_packet.gearsReverse);
}

SCSAPI_VOID onEvent(const scs_event_t event, const void* const info, const scs_context_t) {
  switch (event) {
    case SCS_TELEMETRY_EVENT_paused: setFlag(TF_PAUSED, true); sendPacket(); break;
    case SCS_TELEMETRY_EVENT_started: setFlag(TF_PAUSED, false); break;
    case SCS_TELEMETRY_EVENT_configuration: {
      const scs_telemetry_configuration_t* config = static_cast<const scs_telemetry_configuration_t*>(info);
      if (config && !strcmp(config->id, SCS_TELEMETRY_CONFIG_truck)) onTruckConfig(config->attributes);
      break;
    }
    case SCS_TELEMETRY_EVENT_frame_end: {
      auto now = std::chrono::steady_clock::now();
      if (now - g_lastSend >= std::chrono::microseconds(1000000 / g_settings.rate)) {
        g_lastSend = now;
        sendPacket();
      }
      break;
    }
  }
}

// ---------------------------------------------------------------------
//  Funkcje eksportowane dla gry

PLUGIN_EXPORT SCSAPI_RESULT scs_telemetry_init(const scs_u32_t version, const scs_telemetry_init_params_t* const params) {
  if (version != SCS_TELEMETRY_VERSION_1_01) return SCS_RESULT_unsupported;
  const scs_telemetry_init_params_v101_t* p = static_cast<const scs_telemetry_init_params_v101_t*>(params);
  g_log = p->common.log;

  g_packet = TruckPacket{};
  memcpy(g_packet.magic, TRUCK_PACKET_MAGIC, 4);
  g_packet.version = TRUCK_PACKET_VERSION;
  g_packet.size = sizeof(TruckPacket);
  g_packet.flags = TF_PAUSED;   // telemetria startuje w pauzie
  const char* id = p->common.game_id;
  g_packet.game = !strcmp(id, SCS_GAME_ID_EUT2) ? TRUCK_GAME_ETS2 : !strcmp(id, SCS_GAME_ID_ATS) ? TRUCK_GAME_ATS : TRUCK_GAME_UNKNOWN;

  loadSettings();
  if (!openSocket()) {
    log(SCS_LOG_TYPE_error, "nie udalo sie otworzyc gniazda UDP");
    closeUdp();
    return SCS_RESULT_generic_error;
  }

  const scs_event_t events[] = {SCS_TELEMETRY_EVENT_frame_end, SCS_TELEMETRY_EVENT_paused,
                                SCS_TELEMETRY_EVENT_started, SCS_TELEMETRY_EVENT_configuration};
  for (scs_event_t e : events) {
    if (p->register_for_event(e, onEvent, nullptr) != SCS_RESULT_ok) {
      log(SCS_LOG_TYPE_error, "rejestracja zdarzenia %u nie powiodla sie", e);
      closeUdp();
      return SCS_RESULT_generic_error;
    }
  }
  registerChannels(p);

  log(SCS_LOG_TYPE_message, "wysylam na %s:%d, %d pakietow/s (%s)", g_settings.ip.c_str(), g_settings.port,
      g_settings.rate, p->common.game_name);
  return SCS_RESULT_ok;
}

PLUGIN_EXPORT SCSAPI_VOID scs_telemetry_shutdown(void) {
  // rejestracje zdarzeń i kanałów gra usuwa sama
  setFlag(TF_PAUSED, true);
  sendPacket();
  closeUdp();
  g_log = nullptr;
}
