#ifndef ZAMBRETTI_H
#define ZAMBRETTI_H

#include <Arduino.h>

enum class WeatherForecast {
  SETTLED_FINE,
  FINE_WEATHER,
  BECOMING_FINE,
  FINE_BECOMING_LESS_SETTLED,
  FAIRLY_FINE_SHOWERS_LIKELY,
  SHOWERY_BRIGHT_INTERVALS,
  CHANGEABLE_MENDING,
  SHOWERY_SETTLED_INTERVALS,
  UNSETTLED_PROB_IMPROVING,
  UNSETTLED_RAIN_AT_TIMES,
  VERY_UNSETTLED_RAIN,
  STORMY_MUCH_RAIN,
  SHOWERY_MENDING,
  UNKNOWN
};

class Zambretti {
public:
  // trend: 1 = rising, 0 = steady, -1 = falling
  static WeatherForecast calculate(float pressure, int trend, int month) {
    // Simplified Zambretti Algorithm
    // Input pressure in hPa

    if (trend == 1) { // Rising
      if (pressure > 1030)
        return WeatherForecast::SETTLED_FINE;
      if (pressure > 1020)
        return WeatherForecast::FINE_WEATHER;
      if (pressure > 1010)
        return WeatherForecast::BECOMING_FINE;
      if (pressure > 1000)
        return WeatherForecast::FAIRLY_FINE_SHOWERS_LIKELY;
      return WeatherForecast::SHOWERY_MENDING;
    } else if (trend == -1) { // Falling
      if (pressure > 1030)
        return WeatherForecast::FINE_BECOMING_LESS_SETTLED;
      if (pressure > 1020)
        return WeatherForecast::SHOWERY_BRIGHT_INTERVALS;
      if (pressure > 1010)
        return WeatherForecast::UNSETTLED_RAIN_AT_TIMES;
      if (pressure > 1000)
        return WeatherForecast::VERY_UNSETTLED_RAIN;
      return WeatherForecast::STORMY_MUCH_RAIN;
    } else { // Steady
      if (pressure > 1020)
        return WeatherForecast::FINE_WEATHER;
      if (pressure > 1010)
        return WeatherForecast::CHANGEABLE_MENDING;
      return WeatherForecast::UNSETTLED_RAIN_AT_TIMES;
    }
  }

  static const char *toString(WeatherForecast f) {
    switch (f) {
    case WeatherForecast::SETTLED_FINE:
      return "Settled Fine";
    case WeatherForecast::FINE_WEATHER:
      return "Fine Weather";
    case WeatherForecast::BECOMING_FINE:
      return "Becoming Fine";
    case WeatherForecast::FINE_BECOMING_LESS_SETTLED:
      return "Less Settled";
    case WeatherForecast::FAIRLY_FINE_SHOWERS_LIKELY:
      return "Fair Showers";
    case WeatherForecast::SHOWERY_BRIGHT_INTERVALS:
      return "Showery intervals";
    case WeatherForecast::CHANGEABLE_MENDING:
      return "Changeable";
    case WeatherForecast::UNSETTLED_RAIN_AT_TIMES:
      return "Rainy";
    case WeatherForecast::VERY_UNSETTLED_RAIN:
      return "Unsettled Rain";
    case WeatherForecast::STORMY_MUCH_RAIN:
      return "Stormy";
    case WeatherForecast::SHOWERY_MENDING:
      return "Improving";
    case WeatherForecast::SHOWERY_SETTLED_INTERVALS:
      return "Showery Settled";
    case WeatherForecast::UNSETTLED_PROB_IMPROVING:
      return "Unsettled Improving";
    default:
      return "Variable";
    }
  }
};

#endif // ZAMBRETTI_H
