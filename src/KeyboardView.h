#ifndef KEYBOARD_VIEW_H
#define KEYBOARD_VIEW_H

#include "InputManager.h"
#include <TFT_eSPI.h>


class KeyboardView {
public:
  KeyboardView(TFT_eSPI &tft);
  void handleInput(const ControlState &state);
  void draw();

  String getText() { return _text; }
  void setText(String t) { _text = t; }
  bool isFinished() { return _finished; }
  void reset() {
    _text = "";
    _finished = false;
  }

private:
  TFT_eSPI &_tft;
  String _text;
  int _cursorX, _cursorY;
  int _page; // 0: Lower, 1: Upper, 2: Num/Sym
  bool _finished;

  const char *pages[3] = {"abcdefghijklmnopqrstuvwxyz_-. ",
                          "ABCDEFGHIJKLMNOPQRSTUVWXYZ_-. ",
                          "0123456789!@#$%^&*()+=[]{}<>?"};

  void drawGrid();
  void drawTextBar();
};

#endif // KEYBOARD_VIEW_H
