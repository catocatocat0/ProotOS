/* ============================================================================
 * FACIAL EXPRESSION DEFINITIONS & SELECTOR POOLS
 * ============================================================================
 * Defines the `Face` struct and all configured facial expressions for the
 * Protogen. Each `Face` bundles its idle sprite, blink sprite, visual effect
 * modifier, frame timing, microphone thresholds, and speaking sprite pools.
 * ============================================================================ */

#pragma once

#include <Arduino.h>
#include "drawfaces.h"

// ============================================================================
// FACE SELECTOR CONFIGURATION
// ============================================================================

// Maximum number of facial expressions that can be registered in `faceSelector`.
// Increase this value if you add more than 255 faces.
#define MAX_FACES 255


// ============================================================================
// FACE STRUCT DEFINITION
// ============================================================================

/**
 * Represents a complete facial expression profile.
 *
 * All sprite pointers and pools are nullptr-safe:
 * - Setting `blinkSprite = nullptr` disables blinking for that face.
 * - Setting `modifier = nullptr` disables post-processing effects (aberration/CRT).
 * - Setting `canTalk = false` (or leaving `talkPool` empty / `{ nullptr }`)
 *   disables speaking animations for that face.
 * - Pools with fewer than 255 sprites (e.g., `{ talk1, talk2 }`) have their valid
 *   sprite count automatically detected and cached by `initCounts()`.
 */
struct Face {
  // Visual assets (`drawFace` / `holdFace` image parameters; nullptr-safe)
  const uint32_t* idleSprite;
  const uint32_t* blinkSprite;

  // Display rendering parameters for `drawFace()` and `holdFace()`
  int           blinkHoldTime; // Duration in ms to hold `blinkSprite` in `holdFace()`
  int           offsetX;       // Horizontal pixel offset (`offset_x`)
  int           offsetY;       // Vertical pixel offset (`offset_y`)
  bool          overlay;       // Frame buffer overlay toggle (`overlay`)
  bool          mirror;        // Horizontal panel mirroring toggle (`mirror`)
  FaceModifier* modifier;      // Post-processing effect (`modifier`; nullptr disables effect)

  // Frame timing & speaking toggle
  bool canTalk;
  int  faceDelay;

  // Microphone volume thresholds for this face
  int talkThreshold;
  int loudThreshold;
  int yellThreshold;

  // Speaking animation sprite pools (up to 255 sprites per volume tier)
  const uint32_t* talkPool[255];
  const uint32_t* loudPool[255];
  const uint32_t* yellPool[255];

  // Cached pool sizes (automatically populated by `initCounts()`)
  uint8_t talkCount;
  uint8_t loudCount;
  uint8_t yellCount;

  /**
   * Counts the non-null sprites in each pool and caches the results in
   * `talkCount`, `loudCount`, and `yellCount`. Called once when a face is loaded.
   */
  inline void initCounts() {
    talkCount = 0;
    while (talkCount < 255 && talkPool[talkCount] != nullptr) {
      talkCount++;
    }

    loudCount = 0;
    while (loudCount < 255 && loudPool[loudCount] != nullptr) {
      loudCount++;
    }

    yellCount = 0;
    while (yellCount < 255 && yellPool[yellCount] != nullptr) {
      yellCount++;
    }
  }

  /**
   * Returns a random normal-talking sprite from `talkPool` using cached `talkCount`.
   * Safely falls back to `idleSprite` if `talkPool` is empty.
   */
  inline const uint32_t* talk() const {
    return (talkCount > 0) ? talkPool[random(0, talkCount)] : idleSprite;
  }

  /**
   * Returns a random loud-talking sprite from `loudPool` using cached `loudCount`.
   * Safely falls back to `talk()` if `loudPool` is empty.
   */
  inline const uint32_t* loud() const {
    return (loudCount > 0) ? loudPool[random(0, loudCount)] : talk();
  }

  /**
   * Returns a random yelling sprite from `yellPool` using cached `yellCount`.
   * Safely falls back to `loud()` if `yellPool` is empty.
   */
  inline const uint32_t* yell() const {
    return (yellCount > 0) ? yellPool[random(0, yellCount)] : loud();
  }

  /**
   * Draws a sprite using this face's configured offsets, overlay, mirror, and modifier.
   * Safely does nothing if `sprite` is `nullptr`.
   */
  inline void draw(const uint32_t* sprite) const {
    if (sprite != nullptr) {
      drawFace(offsetX, offsetY, sprite, overlay, mirror, modifier);
    }
  }

  /**
   * Plays the blink animation using `holdFace()` and this face's parameters.
   * Safely does nothing if `blinkSprite` is `nullptr` or `blinkHoldTime <= 0`.
   */
  inline void blink() const {
    if (blinkSprite != nullptr && blinkHoldTime > 0) {
      holdFace(blinkHoldTime, offsetX, offsetY, blinkSprite, overlay, mirror, modifier);
    }
  }
};


// ============================================================================
// FACE SELECTOR STATE & HELPERS
// ============================================================================

// Defined by the user in `src/user/faces.h`
extern const Face* faceSelector[MAX_FACES];

uint8_t numFaces = 0; // Total non-null faces in `faceSelector` (set by `initFaceSelector()`)

/**
 * Counts the non-null `Face` pointers in `faceSelector[]` until it encounters
 * `nullptr` (or reaches `MAX_FACES`). Called once during `setup()`.
 */
inline void initFaceSelector() {
  numFaces = 0;
  while (numFaces < MAX_FACES && faceSelector[numFaces] != nullptr) {
    numFaces++;
  }
}
