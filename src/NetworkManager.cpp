#include "NetworkManager.h"
#include "config.h"

NetworkManager::NetworkManager(HardwareManager &hw) : _hw(hw) {}

void NetworkManager::begin() {
  _prefs.begin("clock-cfg", false);
  _currentConfig = loadConfig();
}

void NetworkManager::saveConfig(const ConfigData &config) {
  _prefs.putString("ssid", config.ssid);
  _prefs.putString("pass", config.pass);
  _prefs.putString("city", config.cityName);
  _prefs.putFloat("lat", config.lat);
  _prefs.putFloat("lon", config.lon);
  _currentConfig = config;
}

ConfigData NetworkManager::loadConfig() {
  ConfigData cfg;
  cfg.ssid = _prefs.getString("ssid", "");
  cfg.pass = _prefs.getString("pass", "");
  cfg.cityName = _prefs.getString("city", "Madrid");
  cfg.lat = _prefs.getFloat("lat", 40.4168f);
  cfg.lon = _prefs.getFloat("lon", -3.7038f);
  return cfg;
}

bool NetworkManager::connect() {
  // Use Preferences SSID if available, otherwise fall back to config.h
  const char *ssid;
  const char *pass;
  if (_currentConfig.ssid.length() > 0) {
    ssid = _currentConfig.ssid.c_str();
    pass = _currentConfig.pass.c_str();
    Serial.printf("[NET] Using saved SSID: %s\n", ssid);
  } else {
    ssid = WIFI_SSID;
    pass = WIFI_PASSWORD;
    Serial.printf("[NET] No saved SSID, using config.h: %s\n", ssid);
  }

  WiFi.begin(ssid, pass);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }
  return WiFi.status() == WL_CONNECTED;
}

bool NetworkManager::syncTime() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[NTP] WiFi not connected, skipping NTP sync.");
    return false;
  }

  // Use POSIX timezone string for automatic DST handling
  Serial.printf("[NTP] Configuring timezone: %s\n", POSIX_TZ);
  configTzTime(POSIX_TZ, NTP_SERVER, "time.nist.gov");

  // Retry loop — fresh WiFi connections may need time to stabilise
  struct tm timeinfo;
  const int maxAttempts = 3;
  bool gotTime = false;

  for (int attempt = 1; attempt <= maxAttempts; attempt++) {
    Serial.printf(
        "[NTP] Attempt %d/%d — waiting for NTP response (10s timeout)...\n",
        attempt, maxAttempts);
    if (getLocalTime(&timeinfo, 10000)) {
      gotTime = true;
      break;
    }
    Serial.printf("[NTP] Attempt %d/%d failed.\n", attempt, maxAttempts);
    if (attempt < maxAttempts) {
      Serial.println("[NTP] Retrying in 2 seconds...");
      delay(2000);
    }
  }

  if (!gotTime) {
    Serial.println("[NTP] ERROR: All NTP attempts failed.");
    return false;
  }

  int year = timeinfo.tm_year + 1900;
  if (year < 2024) {
    Serial.printf(
        "[NTP] ERROR: Invalid year %d received, skipping RTC update.\n", year);
    return false;
  }

  DateTime ntpTime(year, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                   timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  _hw.setRTC(ntpTime);
  Serial.printf("[NTP] RTC updated from NTP: %04d-%02d-%02d %02d:%02d:%02d\n",
                ntpTime.year(), ntpTime.month(), ntpTime.day(), ntpTime.hour(),
                ntpTime.minute(), ntpTime.second());
  return true;
}

bool NetworkManager::geocode(String city, float &lat, float &lon) {
  // Placeholder for actual OpenWeatherMap Geocoding API call
  // Requires an API key which we'll add to config later
  return false;
}

bool NetworkManager::fetchWeather(String &city, float &temp, String &desc) {
  // Placeholder for actual Weather API call
  return false;
}
