#ifndef HARDWARE_MANAGER_H
#define HARDWARE_MANAGER_H

#include <Adafruit_BME280.h>
#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>


struct EnvironmentalData {
  float temperature;
  float humidity;
  float pressure;
  bool valid;
};

class HardwareManager {
public:
  HardwareManager();
  bool begin();

  // Sensor readings
  EnvironmentalData getEnvironmentalData();

  // RTC functions
  DateTime getCurrentTime();
  void setRTC(DateTime dt);

  // Battery/Board info
  float getBatteryVoltage(); // If supported

private:
  Adafruit_BME280 bme;
  RTC_DS3231 rtc;

  bool bmeInitialized;
  bool rtcInitialized;
};

#endif // HARDWARE_MANAGER_H
