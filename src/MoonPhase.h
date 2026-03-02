#ifndef MOON_PHASE_H
#define MOON_PHASE_H

#include <Arduino.h>
#include <RTClib.h>

struct MoonData {
  float illumination; // 0.0 to 1.0
  float age;          // 0 to 29.53
  const char *phaseName;
};

class MoonPhase {
public:
  static MoonData calculate(DateTime dt) {
    // Lunar cycle in days
    const double LUNAR_CYCLE = 29.530588853;

    // Known New Moon: 2000-01-06 18:14 UTC
    DateTime newMoon(2000, 1, 6, 18, 14, 0);

    TimeSpan diff = dt - newMoon;
    double daysSince = diff.totalseconds() / 86400.0;

    double phase = fmod(daysSince, LUNAR_CYCLE);
    double normalize = phase / LUNAR_CYCLE;

    MoonData data;
    data.age = (float)phase;

    // Illumination estimation
    data.illumination = (float)(0.5 * (1.0 - cos(2.0 * PI * normalize)));

    // Phase names
    if (normalize < 0.03)
      data.phaseName = "New Moon";
    else if (normalize < 0.22)
      data.phaseName = "Waxing Crescent";
    else if (normalize < 0.28)
      data.phaseName = "First Quarter";
    else if (normalize < 0.47)
      data.phaseName = "Waxing Gibbous";
    else if (normalize < 0.53)
      data.phaseName = "Full Moon";
    else if (normalize < 0.72)
      data.phaseName = "Waning Gibbous";
    else if (normalize < 0.78)
      data.phaseName = "Last Quarter";
    else if (normalize < 0.97)
      data.phaseName = "Waning Crescent";
    else
      data.phaseName = "New Moon";

    return data;
  }
};

#endif // MOON_PHASE_H
