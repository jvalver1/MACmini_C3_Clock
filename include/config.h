#ifndef CONFIG_H
#define CONFIG_H

/*
// WiFi Configuration Swindon
#define WIFI_SSID "KillZone"
#define WIFI_PASSWORD                                                          \
  "estaesunaclavesegura" // Replace with your actual password
*/

// WiFi Configuration Madrid`
#define WIFI_SSID "MOVISTAR_2B80"
#define WIFI_PASSWORD                                                          \
  "oKyPdM2hYtPBruTyibR4" // Replace with your actual password

// NTP Configuration
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 3600      // CET = UTC+1
#define DAYLIGHT_OFFSET_SEC 3600 // CEST adds +1 hour in summer

// POSIX timezone string for automatic DST handling
// CET-1CEST,M3.5.0,M10.5.0/3 means:
//   CET = standard time name, -1 = UTC+1
//   CEST = DST name, starts last Sunday of March (M3.5.0)
//   ends last Sunday of October at 3:00 (M10.5.0/3)
#define POSIX_TZ "CET-1CEST,M3.5.0,M10.5.0/3"

// Serial Configuration
#define SERIAL_BAUD 9600

// Finnhub API Configuration
// Get your free API key from: https://finnhub.io/
#define FINNHUB_API_KEY "cpbf6g9r01qodesspu1gcpbf6g9r01qodesspu20"

#endif // CONFIG_H
