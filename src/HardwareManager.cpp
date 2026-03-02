#include "HardwareManager.h"

// Define I2C pins for ESP32-C3
#define I2C_SDA 8
#define I2C_SCL 9

HardwareManager::HardwareManager()
    : bmeInitialized(false), rtcInitialized(false) {}

bool HardwareManager::begin() {
  Serial.println("[HW] Starting I2C...");
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.printf("[HW] I2C started on SDA=%d, SCL=%d\n", I2C_SDA, I2C_SCL);

  // Initialize BME280
  Serial.println("[HW] Looking for BME280...");
  if (bme.begin(0x76, &Wire)) {
    bmeInitialized = true;
    Serial.println("[HW] BME280 found at 0x76");
  } else if (bme.begin(0x77, &Wire)) {
    bmeInitialized = true;
    Serial.println("[HW] BME280 found at 0x77");
  } else {
    Serial.println("[HW] BME280 NOT FOUND!");
  }

  // Initialize RTC
  Serial.println("[HW] Looking for DS3231 RTC...");
  if (rtc.begin(&Wire)) {
    rtcInitialized = true;
    Serial.println("[HW] DS3231 found.");
    if (rtc.lostPower()) {
      Serial.println("[HW] RTC lost power, setting default time!");
      rtc.adjust(DateTime(2026, 2, 8, 12, 0, 0));
    }
    DateTime now = rtc.now();
    Serial.printf("[HW] RTC time: %04d-%02d-%02d %02d:%02d:%02d\n", now.year(),
                  now.month(), now.day(), now.hour(), now.minute(),
                  now.second());
  } else {
    Serial.println("[HW] DS3231 NOT FOUND!");
  }

  Serial.printf("[HW] Init result: BME=%s, RTC=%s\n",
                bmeInitialized ? "OK" : "FAIL", rtcInitialized ? "OK" : "FAIL");

  return bmeInitialized && rtcInitialized;
}

EnvironmentalData HardwareManager::getEnvironmentalData() {
  EnvironmentalData data = {0, 0, 0, false};
  if (bmeInitialized) {
    data.temperature = bme.readTemperature();
    data.humidity = bme.readHumidity();
    data.pressure = bme.readPressure() / 100.0F; // hPa
    data.valid = true;
  }
  return data;
}

DateTime HardwareManager::getCurrentTime() {
  if (rtcInitialized) {
    return rtc.now();
  }
  return DateTime(2026, 1, 1, 0, 0, 0);
}

void HardwareManager::setRTC(DateTime dt) {
  if (rtcInitialized) {
    rtc.adjust(dt);
    Serial.printf("[HW] RTC set to: %04d-%02d-%02d %02d:%02d:%02d\n", dt.year(),
                  dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());
  }
}

float HardwareManager::getBatteryVoltage() { return 3.3f; }
