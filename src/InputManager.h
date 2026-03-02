#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "Pins.h"
#include <Arduino.h>

enum class ButtonEvent { NONE, SHORT_PRESS, LONG_PRESS };

struct ControlState {
  ButtonEvent joyUp;
  ButtonEvent joyDown;
  ButtonEvent joyLeft;
  ButtonEvent joyRight;
  ButtonEvent joyCenter;
};

class InputManager {
public:
  InputManager();
  void begin();
  ControlState update();

private:
  ButtonEvent checkButton(int pinIdx, unsigned long &lastTime, bool &lastState,
                          bool &longPressed);

  static const int NUM_INPUTS = 5;
  unsigned long lastTimes[NUM_INPUTS];
  bool lastStates[NUM_INPUTS];
  bool longPressed[NUM_INPUTS];

  const int pins[NUM_INPUTS] = {JOY_UP, JOY_DOWN, JOY_LEFT, JOY_RIGHT,
                                JOY_CLICK};
};

#endif // INPUT_MANAGER_H
