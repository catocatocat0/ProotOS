/* ============================================================================
 * FACIAL POST-PROCESSING EFFECT MODIFIERS
 * ============================================================================
 * Defines the base `FaceModifier` interface and visual effect implementations
 * (such as Chromatic Aberration and CRT scanlines) applied in `drawFace()`.
 * ============================================================================ */

#pragma once

#include <Arduino.h>

// ============================================================================
// BASE MODIFIER INTERFACE
// ============================================================================

/**
 * Base class for defining post-processing special effects for `drawFace()`.
 *
 * - `frameBegin()`    runs once before pixels in a frame are drawn.
 * - `process()`       transforms the current RGB565 pixel in `pixelBuffer` before drawing.
 * - `frameComplete()` runs once after the entire frame has been drawn.
 */
class FaceModifier {
public:
  virtual ~FaceModifier() = default;
  virtual void frameBegin(uint16_t* pixelBuffer, int imgWidth, int imgHeight) {}
  virtual uint16_t process(int index, uint16_t* pixelBuffer, int imgWidth, int imgHeight) = 0;
  virtual void frameComplete(uint16_t* pixelBuffer, int imgWidth, int imgHeight) {}
};


// ============================================================================
// CHROMATIC ABERRATION MODIFIER
// ============================================================================

/**
 * Randomly shifts the Red, Green, and Blue channels by -1 to +1 pixels
 * on each frame to create a glitchy chromatic aberration effect.
 */
class AberrationModifier : public FaceModifier {
public:
  int redOffsetX,   redOffsetY;
  int greenOffsetX, greenOffsetY;
  int blueOffsetX,  blueOffsetY;

  AberrationModifier()
    : redOffsetX(random(-1, 2)),
      redOffsetY(random(-1, 2)),
      greenOffsetX(random(-1, 2)),
      greenOffsetY(random(-1, 2)),
      blueOffsetX(random(-1, 2)),
      blueOffsetY(random(-1, 2)) {}

  void frameBegin(uint16_t* pixelBuffer, int imgWidth, int imgHeight) override {
    // Randomize RGB channel offsets at the start of each frame
    redOffsetX   = random(-1, 2);
    redOffsetY   = random(-1, 2);
    greenOffsetX = random(-1, 2);
    greenOffsetY = random(-1, 2);
    blueOffsetX  = random(-1, 2);
    blueOffsetY  = random(-1, 2);
  }

  uint16_t process(int index, uint16_t* pixelBuffer, int imgWidth, int imgHeight) override {
    int x = index % imgWidth;
    int y = index / imgWidth;

    // Calculate shifted coordinates for each color channel, constrained to image bounds
    int redX   = constrain(x + redOffsetX,   0, imgWidth  - 1);
    int redY   = constrain(y + redOffsetY,   0, imgHeight - 1);
    int greenX = constrain(x + greenOffsetX, 0, imgWidth  - 1);
    int greenY = constrain(y + greenOffsetY, 0, imgHeight - 1);
    int blueX  = constrain(x + blueOffsetX,  0, imgWidth  - 1);
    int blueY  = constrain(y + blueOffsetY,  0, imgHeight - 1);

    // Sample the RGB565 pixel corresponding to each shifted channel
    uint16_t redPixel   = pixelBuffer[redY   * imgWidth + redX];
    uint16_t greenPixel = pixelBuffer[greenY * imgWidth + greenX];
    uint16_t bluePixel  = pixelBuffer[blueY  * imgWidth + blueX];

    // Combine 5-bit Red (0xF800), 6-bit Green (0x07E0), and 5-bit Blue (0x001F) directly
    return (redPixel & 0xF800) | (greenPixel & 0x07E0) | (bluePixel & 0x001F);
  }

  void frameComplete(uint16_t* pixelBuffer, int imgWidth, int imgHeight) override {}
};


// ============================================================================
// CRT SCANLINE MODIFIER
// ============================================================================

/**
 * Simulates a scrolling CRT scanline gradient across the matrix height.
 */
class CRTModifier : public FaceModifier {
public:
  int scanLineOffset;

  CRTModifier() : scanLineOffset(0) {}

  uint16_t process(int index, uint16_t* pixelBuffer, int imgWidth, int imgHeight) override {
    int y = index / imgWidth;

    int displayY = y;
    int distance = (displayY - scanLineOffset + imgHeight) % imgHeight;

    // Generate a 5-pixel-high brightness gradient starting at 85% intensity
    // and stepping down by 15% per row
    if (distance < 5) {
      float    intensity = 0.85f - distance * 0.15f;
      uint16_t pix       = pixelBuffer[index];

      uint16_t r = static_cast<uint16_t>(((pix >> 11) & 0x1F) * intensity);
      uint16_t g = static_cast<uint16_t>(((pix >> 5)  & 0x3F) * intensity);
      uint16_t b = static_cast<uint16_t>((pix         & 0x1F) * intensity);

      return (r << 11) | (g << 5) | b;
    }

    return pixelBuffer[index];
  }

  void frameComplete(uint16_t* pixelBuffer, int imgWidth, int imgHeight) override {
    // Advance the scanline gradient vertically after each completed frame
    scanLineOffset = (scanLineOffset + 1) % imgHeight;
  }
};
