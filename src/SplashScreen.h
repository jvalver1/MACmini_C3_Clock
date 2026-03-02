#ifndef SPLASH_SCREEN_H
#define SPLASH_SCREEN_H

#include "ScreenManager.h"

// States for the splash carousel
enum class SplashState {
  LOGO,           // Logo with text overlay, auto-advances
  TRANSITIONING,  // Animated wipe to next image
  MONTYKONA,      // Montykona artist image
  TRANSITIONING2, // Animated wipe from Montykona to Holistica
  HOLISTICA       // Holistica artist image
};

// Number of available random transitions
#define NUM_TRANSITIONS 6

class SplashScreen : public Screen {
public:
  // Time before the logo auto-advances (no user input)
  static constexpr uint32_t SPLASH_DURATION_MS = 3000;
  // Delay between each transition animation step (ms)
  static constexpr uint32_t TRANSITION_DELAY_MS = 15;

  SplashScreen(TFT_eSPI &tft);
  void onEntry() override;
  void update(unsigned long now) override;
  void draw(TFT_eSPI &tft) override;
  void handleInput(const ControlState &state) override;

  bool isFinished() const { return _finished; }

private:
  TFT_eSPI &_tft;
  SplashState _state;
  bool _finished;
  unsigned long _startTime; // auto-advance timer for LOGO state

  // Transition engine state
  const uint16_t *_nextImg; // target image for the current transition
  int _transitionType;
  int _transitionStep;
  int _transitionMaxSteps;
  unsigned long _lastStepTime;
  bool _transitionComplete;

  // Drawing helpers
  void drawImage(TFT_eSPI &tft, const uint16_t *img);
  void drawLogoWithText(TFT_eSPI &tft);

  // Transition engine
  void startTransition(const uint16_t *targetImg);
  void stepTransition(TFT_eSPI &tft);

  void transHorizontalWipe(TFT_eSPI &tft);
  void transVerticalBlinds(TFT_eSPI &tft);
  void transDissolve(TFT_eSPI &tft);
  void transCircleReveal(TFT_eSPI &tft);
  void transDiagonalWipe(TFT_eSPI &tft);
  void transCheckerboard(TFT_eSPI &tft);
};

#endif // SPLASH_SCREEN_H
