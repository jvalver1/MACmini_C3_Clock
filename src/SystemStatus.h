#ifndef SYSTEM_STATUS_H
#define SYSTEM_STATUS_H

#include "HardwareManager.h"
#include "InputManager.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

struct HourlyWeather {
  int hour;
  float temperature;
  int precipitationProbability;
  int cloudCover;
  int weatherCode;
  bool valid;
};

struct WeatherData {
  String city;
  String description;
  float temperature;
  int weatherCode;
  bool isDay;
  int precipitationProbability;
  float rain;
  float showers;
  float snowfall;
  int cloudCover;
  float pressure;
  float windSpeed;
  int windDirection;
  HourlyWeather hourly[2];
  bool valid;
};

struct SystemStatus {
  DateTime currentTime;
  EnvironmentalData envData;
  bool wifiConnected;
  bool ntpSynced;
  String weatherDesc;
  float weatherTemp;
  WeatherData weather;
  bool weatherValid;
  float batteryVoltage;
};

// Global synchronization variables
extern SemaphoreHandle_t xStatusMutex;
extern QueueHandle_t xQueueInputEvents;
extern SystemStatus sysStatus;

#endif // SYSTEM_STATUS_H
