---
name: Mac Mini C3 TFT Display & UI Guide
description: Crucial documentation and guidelines for rendering graphics and interfaces on the ESP32-C3 Mac Mini smart clock, detailing the ST7735 colour inversion bug, Sprite-based flicker-free rendering, and custom TrueType font generation.
---

# UI Development Guide: MACmini C3 TFT Display

This skill document defines the guidelines and best practices for creating new visual interfaces and extending the display functionality of the Mac Mini C3 Smart Clock. 

## 1. Hardware & Core Driver
- **Resolution**: 128 (width) x 160 (height) pixels (Portrait Orientation).
- **Driver**: ST7735 (configured via `TFT_eSPI` library).
- **Specific Tab**: `ST7735_GREENTAB`.
- **Connectivity**: ESP32-C3 communicating over hardware SPI.

## 2. Colour Caveat: The BGR Byte Swap Bug
Due to physical design variations across ST7735-based screens, this specific display expects colour data in BGR format rather than the standard RGB. 
Historically, attempting to override this using `platformio.ini` variables (e.g., `-DTFT_RGB_ORDER=TFT_BGR`) can easily lead to conflicts or be outright ignored by the hardware during dynamic drawing.
Because of this, standard Hex macros output inverted colors. For instance: `0xFFE0` (Yellow) will render as Cyan (Blue), and a Red gradient will render as a Blue gradient.

### **The Solution (Software Side)**
Always apply the explicit `HW_COLOR` byte-flipping macro when defining UI colours to guarantee complete immunity against hardware inversions.
```cpp
// Flips 'b' and 'r' during 16-bit 565 compilation
inline uint16_t HW_COLOR(uint8_t r, uint8_t g, uint8_t b) {
  return (((b & 0xF8) << 8) | ((g & 0xFC) << 3) | (r >> 3));
}
```
When creating palettes, use:
```cpp
#define COL_BRIGHT_YELLOW HW_COLOR(255, 255,   0)
#define COL_DEEP_NAVY     HW_COLOR(  0,  20, 150)
```

## 3. Rendering: Eliminating Screen Flicker
Direct hardware draw calls (like `tft.fillRect()` or `tft.drawString()`) execute visibly on the screen. Because the display updates over standard SPI, redrawing the screen elements sequentially will cause noticeable "screen tearing" or flickering.

### **The Solution (TFT_eSprite In-Memory Buffers)**
To establish a perfectly smooth UI (especially for rapid 1-second interval redraws):
1. Create a `TFT_eSprite` in RAM sized to the bounds of the changing UI element. 
2. Paint the background (gradients, solids) into the sprite first.
3. Draw the dynamic text over it.
4. Push the fully-baked image directly to the physical display using `spr.pushSprite(X, Y)`. 

```cpp
TFT_eSprite spr = TFT_eSprite(&tft);
spr.setColorDepth(16);
spr.createSprite(WIDTH, HEIGHT);
// ... draw completely inside spr (spr.drawPixel, spr.drawString)
spr.pushSprite(START_X, START_Y);
spr.deleteSprite(); // Critical: ALWAYS free memory
```

## 4. Custom Proportional Font Generation
The default `TFT_eSPI` fonts are monospaced and frequently scale poorly to extreme portrait interfaces (for example, attempting to fit `08:15:22` horizontally).
To solve this, there is a dedicated Python font generator located at `tools/gen_font.py` which dynamically crops and embeds highly-compressed system fonts into an Array.

### **Features of `gen_font.py`:**
- Connects directly into local TrueType formats (`impact.ttf`) using the `Pillow` library.
- Proportionally evaluates and shrinks specific characters (e.g. shrinking colons `:` down to 60% of the overall height, centering them).
- Sets manual individual character widths (e.g., Numbers = 15px, Colon = 8px).
- Bakes the result into bitwise 1D arrays inside `src/TallFont.h` which can be efficiently painted using `spr.drawPixel` loops for pixel-perfect bespoke UI components.

When designing new UIs, do not rely on standard font scaling if real-estate is tight. Expand `gen_font.py` for new typographies!

## 5. Screen Lifecycle (Architecture)
All screens must implement the base UI `Screen` loop. When drafting new screens:
- Store drawing bounds and state natively inside the Screen class instance.
- Only calculate visual transitions exactly when `update(...)` triggers changes.
- In `draw(...)`, use boolean guards (`if (_needsRedraw)`) to avoid pushing unnecessary sprites to the display if the state hasn't moved. This keeps the C3 processor heavily optimized and cool.
