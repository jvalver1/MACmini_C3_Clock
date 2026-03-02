#include "SplashScreen.h"
#include "HolisticaImg.h"
#include "LogoImg.h"
#include "MontykonaImg.h"
#include <pgmspace.h>

// Display dimensions
#define SCREEN_W 128
#define SCREEN_H 160

// ─── Constructor ─────────────────────────────────────────────────────────────

SplashScreen::SplashScreen(TFT_eSPI &tft)
    : _tft(tft), _state(SplashState::LOGO), _finished(false), _startTime(0),
      _nextImg(nullptr), _transitionType(0), _transitionStep(0),
      _transitionMaxSteps(0), _lastStepTime(0), _transitionComplete(false) {}

// ─── onEntry ─────────────────────────────────────────────────────────────────

void SplashScreen::onEntry() {
  Serial.println("[SPLASH] onEntry — drawing logo with text");
  _state = SplashState::LOGO;
  _finished = false;
  _transitionComplete = false;
  _startTime = millis();
  drawLogoWithText(_tft);
}

// ─── drawLogoWithText ────────────────────────────────────────────────────────

void SplashScreen::drawLogoWithText(TFT_eSPI &tft) {
  // Draw the full logo image first
  drawImage(tft, logoImg);

  // --- Version number at the top ---
  tft.setSwapBytes(false);
  // Dark banner behind top text for legibility
  tft.fillRect(0, 0, SCREEN_W, 14, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TC_DATUM); // top-centre
  tft.drawString("v.06", SCREEN_W / 2, 2, 2);

  // --- Copyright at the bottom ---
  tft.fillRect(0, SCREEN_H - 14, SCREEN_W, 14, TFT_BLACK);
  tft.setTextDatum(BC_DATUM); // bottom-centre
  tft.drawString("2026 Montykona Inc.", SCREEN_W / 2, SCREEN_H - 2, 1);
}

// ─── drawImage ───────────────────────────────────────────────────────────────

void SplashScreen::drawImage(TFT_eSPI &tft, const uint16_t *img) {
  tft.setSwapBytes(true);
  uint16_t lineBuf[SCREEN_W];
  for (int y = 0; y < SCREEN_H; y++) {
    memcpy_P(lineBuf, &img[y * SCREEN_W], SCREEN_W * sizeof(uint16_t));
    tft.pushImage(0, y, SCREEN_W, 1, lineBuf);
  }
  tft.setSwapBytes(false);
}

// ─── update ──────────────────────────────────────────────────────────────────

void SplashScreen::update(unsigned long now) {
  // Auto-advance from logo only
  if (_state == SplashState::LOGO && !_finished) {
    if (now - _startTime >= SPLASH_DURATION_MS) {
      _finished = true;
      Serial.println("[SPLASH] Auto-advance timer expired — going to Clock");
      return;
    }
  }

  // Run transition animation
  if ((_state == SplashState::TRANSITIONING ||
       _state == SplashState::TRANSITIONING2) &&
      !_transitionComplete) {
    if (now - _lastStepTime >= TRANSITION_DELAY_MS) {
      _lastStepTime = now;
      stepTransition(_tft);
    }
  }
}

// ─── draw ────────────────────────────────────────────────────────────────────

void SplashScreen::draw(TFT_eSPI &tft) {
  // Static content drawn in onEntry / after transitions complete
  (void)tft;
}

// ─── handleInput ─────────────────────────────────────────────────────────────

void SplashScreen::handleInput(const ControlState &state) {
  if (state.joyCenter != ButtonEvent::SHORT_PRESS)
    return;

  switch (_state) {
  case SplashState::LOGO:
    // Start carousel: Logo → Montykona with random transition
    Serial.println("[SPLASH] JoyCenter — Logo -> Montykona transition");
    startTransition(montykonaImg);
    _state = SplashState::TRANSITIONING;
    break;

  case SplashState::MONTYKONA:
    // Carousel: Montykona → Holistica with random transition
    Serial.println("[SPLASH] JoyCenter — Montykona -> Holistica transition");
    startTransition(holisticaImg);
    _state = SplashState::TRANSITIONING2;
    break;

  case SplashState::HOLISTICA:
    // End of carousel: go to Clock
    Serial.println("[SPLASH] JoyCenter — Holistica -> finished");
    _finished = true;
    break;

  default:
    // Ignore presses during active transitions
    break;
  }
}

// ─── Transition Engine ───────────────────────────────────────────────────────

void SplashScreen::startTransition(const uint16_t *targetImg) {
  _nextImg = targetImg;
  _transitionType = random(0, NUM_TRANSITIONS);
  _transitionStep = 0;
  _transitionComplete = false;
  _lastStepTime = millis();

  switch (_transitionType) {
  case 0:
    _transitionMaxSteps = SCREEN_W;
    break; // Horizontal wipe
  case 1:
    _transitionMaxSteps = SCREEN_W / 8;
    break; // Vertical blinds
  case 2:
    _transitionMaxSteps = 80;
    break; // Dissolve
  case 3:
    _transitionMaxSteps = 120;
    break; // Circle reveal
  case 4:
    _transitionMaxSteps = SCREEN_W + SCREEN_H;
    break; // Diagonal wipe
  case 5:
    _transitionMaxSteps = 32;
    break; // Checkerboard
  }

  Serial.printf("[SPLASH] Transition type=%d, maxSteps=%d\n", _transitionType,
                _transitionMaxSteps);
}

void SplashScreen::stepTransition(TFT_eSPI &tft) {
  if (_transitionComplete)
    return;

  tft.setSwapBytes(true);

  switch (_transitionType) {
  case 0:
    transHorizontalWipe(tft);
    break;
  case 1:
    transVerticalBlinds(tft);
    break;
  case 2:
    transDissolve(tft);
    break;
  case 3:
    transCircleReveal(tft);
    break;
  case 4:
    transDiagonalWipe(tft);
    break;
  case 5:
    transCheckerboard(tft);
    break;
  }

  _transitionStep++;

  if (_transitionStep >= _transitionMaxSteps) {
    _transitionComplete = true;

    // Finalise — draw full target image cleanly
    drawImage(tft, _nextImg);

    if (_state == SplashState::TRANSITIONING) {
      _state = SplashState::MONTYKONA;
      Serial.println("[SPLASH] Transition done — showing Montykona");
    } else {
      _state = SplashState::HOLISTICA;
      Serial.println("[SPLASH] Transition done — showing Holistica");
    }
  }

  tft.setSwapBytes(false);
}

// ─── Transition Implementations ──────────────────────────────────────────────

// 0: Horizontal Wipe — reveal one column at a time, left to right
void SplashScreen::transHorizontalWipe(TFT_eSPI &tft) {
  int x = _transitionStep;
  if (x >= SCREEN_W)
    return;
  uint16_t colBuf[SCREEN_H];
  for (int y = 0; y < SCREEN_H; y++) {
    colBuf[y] = pgm_read_word(&_nextImg[y * SCREEN_W + x]);
  }
  tft.pushImage(x, 0, 1, SCREEN_H, colBuf);
}

// 1: Vertical Blinds — 8 vertical strips reveal simultaneously
void SplashScreen::transVerticalBlinds(TFT_eSPI &tft) {
  int blindWidth = SCREEN_W / 8;
  int step = _transitionStep;
  if (step >= blindWidth)
    return;
  uint16_t colBuf[SCREEN_H];
  for (int blind = 0; blind < 8; blind++) {
    int x = blind * blindWidth + step;
    if (x >= SCREEN_W)
      continue;
    for (int y = 0; y < SCREEN_H; y++) {
      colBuf[y] = pgm_read_word(&_nextImg[y * SCREEN_W + x]);
    }
    tft.pushImage(x, 0, 1, SCREEN_H, colBuf);
  }
}

// 2: Dissolve — random 4×4 blocks progressively become the next image
void SplashScreen::transDissolve(TFT_eSPI &tft) {
  const int blockW = 4, blockH = 4;
  int gridW = SCREEN_W / blockW;
  int gridH = SCREEN_H / blockH;
  int totalBlocks = gridW * gridH;
  int blocksPerStep = totalBlocks / _transitionMaxSteps + 1;
  uint16_t blockBuf[blockW * blockH];

  for (int i = 0; i < blocksPerStep; i++) {
    int idx = ((_transitionStep * blocksPerStep + i) * 997) % totalBlocks;
    int bx = (idx % gridW) * blockW;
    int by = (idx / gridW) * blockH;
    for (int dy = 0; dy < blockH; dy++) {
      for (int dx = 0; dx < blockW; dx++) {
        int srcY = by + dy, srcX = bx + dx;
        if (srcX < SCREEN_W && srcY < SCREEN_H) {
          blockBuf[dy * blockW + dx] =
              pgm_read_word(&_nextImg[srcY * SCREEN_W + srcX]);
        }
      }
    }
    tft.pushImage(bx, by, blockW, blockH, blockBuf);
  }
}

// 3: Circle Reveal — expanding circle from centre
void SplashScreen::transCircleReveal(TFT_eSPI &tft) {
  int cx = SCREEN_W / 2, cy = SCREEN_H / 2;
  float maxR = sqrt((float)(cx * cx + cy * cy));
  float r = (maxR * (_transitionStep + 1)) / _transitionMaxSteps;
  float rPrev = (_transitionStep > 0)
                    ? (maxR * _transitionStep) / _transitionMaxSteps
                    : 0.0f;
  float r2 = r * r, rPrev2 = rPrev * rPrev;
  int yMin = max(0, (int)(cy - r));
  int yMax = min(SCREEN_H - 1, (int)(cy + r));
  uint16_t lineBuf[SCREEN_W];

  for (int y = yMin; y <= yMax; y++) {
    float dy = (float)(y - cy), dy2 = dy * dy;
    bool hasPixels = false;
    int xStart = SCREEN_W, xEnd = 0;
    for (int x = 0; x < SCREEN_W; x++) {
      float dx = (float)(x - cx);
      float dist2 = dx * dx + dy2;
      if (dist2 <= r2 && dist2 > rPrev2) {
        lineBuf[x] = pgm_read_word(&_nextImg[y * SCREEN_W + x]);
        if (x < xStart)
          xStart = x;
        if (x > xEnd)
          xEnd = x;
        hasPixels = true;
      }
    }
    if (hasPixels) {
      tft.pushImage(xStart, y, xEnd - xStart + 1, 1, &lineBuf[xStart]);
    }
  }
}

// 4: Diagonal Wipe — sweeps from top-left to bottom-right
void SplashScreen::transDiagonalWipe(TFT_eSPI &tft) {
  int diag = _transitionStep;
  uint16_t pixel;
  for (int step = 0; step < 2; step++) {
    int d = diag * 2 + step;
    for (int y = 0; y < SCREEN_H; y++) {
      int x = d - y;
      if (x >= 0 && x < SCREEN_W) {
        pixel = pgm_read_word(&_nextImg[y * SCREEN_W + x]);
        tft.pushImage(x, y, 1, 1, &pixel);
      }
    }
  }
}

// 5: Checkerboard — alternating 8×8 tiles flip to the next image
void SplashScreen::transCheckerboard(TFT_eSPI &tft) {
  const int tileW = 8, tileH = 8;
  int gridW = SCREEN_W / tileW;
  int gridH = SCREEN_H / tileH;
  int totalTiles = gridW * gridH;
  int tilesPerStep = totalTiles / _transitionMaxSteps + 1;
  uint16_t tileBuf[tileW * tileH];

  for (int i = 0; i < tilesPerStep; i++) {
    int idx = ((_transitionStep * tilesPerStep + i) * 179) % totalTiles;
    int tx = (idx % gridW) * tileW;
    int ty = (idx / gridW) * tileH;
    for (int dy = 0; dy < tileH; dy++) {
      for (int dx = 0; dx < tileW; dx++) {
        int srcX = tx + dx, srcY = ty + dy;
        if (srcX < SCREEN_W && srcY < SCREEN_H) {
          tileBuf[dy * tileW + dx] =
              pgm_read_word(&_nextImg[srcY * SCREEN_W + srcX]);
        }
      }
    }
    tft.pushImage(tx, ty, tileW, tileH, tileBuf);
  }
}
