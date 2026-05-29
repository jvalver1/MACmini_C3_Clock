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
  cfg.cityName = _prefs.getString("city", "Swindon");
  cfg.lat = _prefs.getFloat("lat", 51.55797f);
  cfg.lon = _prefs.getFloat("lon", -1.78116f);
  return cfg;
}

bool NetworkManager::connect() {
  WiFi.mode(WIFI_STA);

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

  WiFi.disconnect(false);
  WiFi.begin(ssid, pass);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }
  bool connected = WiFi.status() == WL_CONNECTED;
  if (connected) {
    Serial.printf("[NET] Connected. IP=%s RSSI=%d dBm\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
  } else {
    Serial.printf("[NET] Connection failed. WiFi status=%d\n", WiFi.status());
  }
  return connected;
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
  if (city.equalsIgnoreCase("Swindon") ||
      city.equalsIgnoreCase("Swindon UK") ||
      city.equalsIgnoreCase("Swindon, UK")) {
    lat = 51.55797f;
    lon = -1.78116f;
    return true;
  }
  return false;
}

bool NetworkManager::fetchWeather(String &city, float &temp, String &desc) {
  WeatherData weather;
  bool ok = fetchWeather(weather);
  if (ok) {
    city = weather.city;
    temp = weather.temperature;
    desc = weather.description;
  }
  return ok;
}

namespace {
const char *weatherDescription(int code, bool isDay) {
  switch (code) {
  case 0:
    return isDay ? "Sunny" : "Clear Night";
  case 1:
  case 2:
    return isDay ? "Partly Cloudy" : "Partly Cloudy Night";
  case 3:
    return isDay ? "Cloudy" : "Cloudy Night";
  case 45:
  case 48:
    return "Fog";
  case 51:
  case 53:
  case 55:
  case 56:
  case 57:
  case 61:
  case 63:
    return "Rain";
  case 65:
  case 66:
  case 67:
  case 80:
  case 81:
  case 82:
    return "Heavy Rain";
  case 71:
  case 73:
    return "Snow";
  case 75:
  case 77:
    return "Heavy Snow";
  case 85:
  case 86:
    return "Rain & Snow";
  case 95:
  case 96:
  case 99:
    return "Thunderstorm";
  default:
    return "Weather";
  }
}

int parseHour(const char *isoTime) {
  if (isoTime == nullptr || strlen(isoTime) < 13) {
    return 0;
  }
  return (isoTime[11] - '0') * 10 + (isoTime[12] - '0');
}
} // namespace

bool NetworkManager::fetchWeather(WeatherData &weather) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WEATHER] WiFi not connected.");
    return false;
  }

  ConfigData cfg = loadConfig();
  if (cfg.cityName.equalsIgnoreCase("Swindon") ||
      cfg.cityName.equalsIgnoreCase("Swindon UK") ||
      cfg.cityName.equalsIgnoreCase("Swindon, UK")) {
    cfg.cityName = "Swindon";
    cfg.lat = 51.55797f;
    cfg.lon = -1.78116f;
  }

  String url = "http://api.open-meteo.com/v1/forecast";
  url += "?latitude=" + String(cfg.lat, 5);
  url += "&longitude=" + String(cfg.lon, 5);
  url += "&current=temperature_2m,precipitation_probability,rain,showers,";
  url += "snowfall,weather_code,cloud_cover,pressure_msl,wind_speed_10m,";
  url += "wind_direction_10m,is_day";
  url += "&hourly=temperature_2m,precipitation_probability,cloud_cover,weather_code";
  url += "&forecast_hours=3&timezone=Europe%2FLondon";

  HTTPClient http;
  http.setTimeout(12000);
  http.setReuse(false);
  http.useHTTP10(true);
  Serial.printf("[WEATHER] GET %s\n", url.c_str());
  http.begin(url);
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[WEATHER] Open-Meteo request failed: HTTP %d\n", code);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();
  Serial.printf("[WEATHER] Payload bytes: %u\n", payload.length());

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("[WEATHER] JSON parse failed: %s\n", err.c_str());
    return false;
  }

  JsonObject current = doc["current"];
  JsonObject hourly = doc["hourly"];
  if (current.isNull()) {
    Serial.println("[WEATHER] Missing current weather data.");
    return false;
  }

  weather = WeatherData();
  weather.city = cfg.cityName;
  weather.temperature = current["temperature_2m"] | 0.0f;
  weather.weatherCode = current["weather_code"] | 0;
  weather.isDay = (current["is_day"] | 1) == 1;
  weather.precipitationProbability =
      current["precipitation_probability"] | -1;
  weather.rain = current["rain"] | 0.0f;
  weather.showers = current["showers"] | 0.0f;
  weather.snowfall = current["snowfall"] | 0.0f;
  weather.cloudCover = current["cloud_cover"] | 0;
  weather.pressure = current["pressure_msl"] | 0.0f;
  weather.windSpeed = current["wind_speed_10m"] | 0.0f;
  weather.windDirection = current["wind_direction_10m"] | 0;
  weather.description =
      weatherDescription(weather.weatherCode, weather.isDay);

  JsonArray times = hourly["time"].as<JsonArray>();
  JsonArray temps = hourly["temperature_2m"].as<JsonArray>();
  JsonArray probs = hourly["precipitation_probability"].as<JsonArray>();
  JsonArray clouds = hourly["cloud_cover"].as<JsonArray>();
  JsonArray codes = hourly["weather_code"].as<JsonArray>();

  for (int i = 0; i < 2; i++) {
    int src = i + 1;
    if (times.size() <= src && times.size() > i) {
      src = i;
    }
    if (temps.size() > src && probs.size() > src && clouds.size() > src &&
        codes.size() > src) {
      const char *timeText = times[src] | "";
      weather.hourly[i].hour = parseHour(timeText);
      weather.hourly[i].temperature = temps[src] | 0.0f;
      weather.hourly[i].precipitationProbability = probs[src] | 0;
      weather.hourly[i].cloudCover = clouds[src] | 0;
      weather.hourly[i].weatherCode = codes[src] | 0;
      weather.hourly[i].valid = true;
    }
  }

  if (weather.precipitationProbability < 0) {
    weather.precipitationProbability =
        weather.hourly[0].valid ? weather.hourly[0].precipitationProbability : 0;
  }

  weather.valid = true;
  Serial.printf("[WEATHER] %s: %.1fC, %s\n", weather.city.c_str(),
                weather.temperature, weather.description.c_str());
  return true;
}

int NetworkManager::scanNetworks(String results[], int maxCount) {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  Serial.println("[NET] Starting WiFi scan...");
  int found = WiFi.scanNetworks();
  Serial.printf("[NET] Scan found %d networks\n", found);

  int count = min(found, maxCount);
  for (int i = 0; i < count; i++) {
    results[i] = WiFi.SSID(i);
  }
  WiFi.scanDelete();
  return count;
}
