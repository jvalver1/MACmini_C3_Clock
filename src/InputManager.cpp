#include "InputManager.h"

#define DEBOUNCE_MS 50
#define LONG_PRESS_MS 1000

InputManager::InputManager() {
  for (int i = 0; i < NUM_INPUTS; i++) {
    lastTimes[i] = 0;
    lastStates[i] = true; // Pin high by default
    longPressed[i] = false;
  }
}

void InputManager::begin() {
  for (int i = 0; i < NUM_INPUTS; i++) {
    Serial.printf("[INPUT] pinMode(%d, INPUT_PULLUP)\n", pins[i]);
    pinMode(pins[i], INPUT_PULLUP);
  }
  Serial.println("[INPUT] All pins initialized.");
}

ButtonEvent InputManager::checkButton(int pinIdx, unsigned long &lastTime,
                                      bool &lastState, bool &longPressedFlag) {
  int pin = pins[pinIdx];
  bool currentState = digitalRead(pin);
  ButtonEvent event = ButtonEvent::NONE;
  unsigned long now = millis();

  if (currentState != lastState) {
    if (now - lastTime > DEBOUNCE_MS) {
      if (lastState == LOW && currentState == HIGH) {
        // Button released
        if (!longPressedFlag) {
          event = ButtonEvent::SHORT_PRESS;
        }
        longPressedFlag = false;
      }
      lastState = currentState;
      lastTime = now;
    }
  }

  if (currentState == LOW && !longPressedFlag) {
    if (now - lastTime > LONG_PRESS_MS) {
      longPressedFlag = true;
      event = ButtonEvent::LONG_PRESS;
    }
  }

  return event;
}

ControlState InputManager::update() {
  ControlState state;
  state.joyUp = checkButton(0, lastTimes[0], lastStates[0], longPressed[0]);
  state.joyDown = checkButton(1, lastTimes[1], lastStates[1], longPressed[1]);
  state.joyLeft = checkButton(2, lastTimes[2], lastStates[2], longPressed[2]);
  state.joyRight = checkButton(3, lastTimes[3], lastStates[3], longPressed[3]);
  state.joyCenter = checkButton(4, lastTimes[4], lastStates[4], longPressed[4]);

  return state;
}
