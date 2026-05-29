#ifndef ENV_SCREEN_H
#define ENV_SCREEN_H

#include "Icons.h"
#include "ScreenManager.h"


class EnvScreen : public Screen {
public:
  EnvScreen(TFT_eSPI &tft, HardwareManager &hw);
  ~EnvScreen();
  void onEntry() override;
  void onExit() override;
  void update(unsigned long now) override;
  void draw(TFT_eSPI &tft) override;
  void handleInput(const ControlState &state) override;

private:
  TFT_eSPI &_tft;
  HardwareManager &_hw;
  EnvironmentalData _data;
  DateTime _time;
  TFT_eSprite _screenSprite;
  unsigned long _lastUpdate;
  int _lastMinute;
  bool _needsRedraw;
  bool _spriteCreated;

  void createSprite();
  void deleteSprite();
  void drawBackground(TFT_eSprite &spr);
  void drawGaugeArc(TFT_eSprite &spr, int cx, int cy, int radius,
                    int startDeg, int endDeg, uint16_t color, int thickness);
  void drawThermometerIcon(TFT_eSprite &spr, int x, int y, uint16_t color);
  void drawHumidityIcon(TFT_eSprite &spr, int x, int y, uint16_t color);
  void drawPressureIcon(TFT_eSprite &spr, int x, int y, uint16_t color);
};

#endif // ENV_SCREEN_H
