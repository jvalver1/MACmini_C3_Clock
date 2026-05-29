#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "HardwareManager.h"
#include "SystemStatus.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFi.h>

struct ConfigData {
  String ssid;
  String pass;
  String cityName;
  float lat;
  float lon;
};

class NetworkManager {
public:
  NetworkManager(HardwareManager &hw);
  void begin();

  // Config management
  void saveConfig(const ConfigData &config);
  ConfigData loadConfig();

  // Connectivity
  bool connect();
  bool syncTime();
  int scanNetworks(String results[], int maxCount);

  // API Fetches
  bool fetchWeather(String &city, float &temp, String &desc);
  bool fetchWeather(WeatherData &weather);
  bool geocode(String city, float &lat, float &lon);

private:
  HardwareManager &_hw;
  Preferences _prefs;
  ConfigData _currentConfig;
};

#endif // NETWORK_MANAGER_H
