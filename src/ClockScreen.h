#ifndef CLOCK_SCREEN_H
#define CLOCK_SCREEN_H

#include "ScreenManager.h"
#include <RTClib.h>

enum class ClockTheme { GRADIENT = 0, SLEEK };

class ClockScreen : public Screen {
public:
  ClockScreen(TFT_eSPI &tft, HardwareManager &hw);
  ~ClockScreen() override;
  void onEntry() override;
  void onExit() override;
  void update(unsigned long now) override;
  void draw(TFT_eSPI &tft) override;
  void handleInput(const ControlState &state) override;

private:
  void drawBackground(TFT_eSPI &tft);
  void createSprites();
  void deleteSprites();

  TFT_eSPI &_tft;
  HardwareManager &_hw;
  DateTime _lastTime;
  bool _needsFullRedraw;
  bool _needsTimeRedraw;
  int _lastSecond;
  int _lastMinute;
  ClockTheme _theme;

  // Persistent TFT_eSprite buffers to avoid heap fragmentation
  TFT_eSprite _timeSprite;
  TFT_eSprite _sprH;
  TFT_eSprite _sprM;
  TFT_eSprite _sprS;
  bool _spritesCreated;

  // Precomputed Gradient Color Lookup Table (0 to 120 pixels from center)
  uint16_t _gradColors[121];
};

#endif // CLOCK_SCREEN_H
