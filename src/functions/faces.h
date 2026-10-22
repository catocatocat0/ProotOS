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
 * - Pools with fewer than 5 sprites (e.g., `{ talk1, talk2 }`) have their valid
 *   sprite count automatically detected and cached by `initCounts()`.
 */
struct Face {
  // Visual assets & post-processing effect
  const uint32_t* idleSprite;
  const uint32_t* blinkSprite;
  FaceModifier*   modifier;

  // Frame timing & speaking toggle
  bool canTalk;
  int  faceDelay;

  // Microphone volume thresholds for this face
  int talkThreshold;
  int loudThreshold;
  int yellThreshold;

  // Speaking animation sprite pools (up to 5 sprites per volume tier)
  const uint32_t* talkPool[5];
  const uint32_t* loudPool[5];
  const uint32_t* yellPool[5];

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
    while (talkCount < 5 && talkPool[talkCount] != nullptr) {
      talkCount++;
    }

    loudCount = 0;
    while (loudCount < 5 && loudPool[loudCount] != nullptr) {
      loudCount++;
    }

    yellCount = 0;
    while (yellCount < 5 && yellPool[yellCount] != nullptr) {
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
};


// ============================================================================
// INDIVIDUAL FACE CONFIGURATIONS
// ============================================================================

// Standard expression with full talking, loud, and yelling sprite pools
const Face normalFace = {
  .idleSprite    = normal,
  .blinkSprite   = blink,
  .modifier      = abberation,
  .canTalk       = true,
  .faceDelay     = 100,
  .talkThreshold = 300,
  .loudThreshold = 600,
  .yellThreshold = 1500,
  .talkPool      = { talk1, talk2, talk3, talk4, talk5 },
  .loudPool      = { loud1, loud2, loud3, loud4, loud5 },
  .yellPool      = { yell1, yell2, yell3, yell4, yell5 }
};

// Smug expression (talking disabled; pools left empty / nullptr)
const Face smugFace = {
  .idleSprite    = smug,
  .blinkSprite   = blink,
  .modifier      = abberation,
  .canTalk       = false,
  .faceDelay     = 100,
  .talkThreshold = 300,
  .loudThreshold = 600,
  .yellThreshold = 1500,
  .talkPool      = { nullptr },
  .loudPool      = { nullptr },
  .yellPool      = { nullptr }
};

// Silent normal expression (same idle sprite as normalFace, but talking disabled)
const Face normalSilentFace = {
  .idleSprite    = normal,
  .blinkSprite   = blink,
  .modifier      = abberation,
  .canTalk       = false,
  .faceDelay     = 100,
  .talkThreshold = 300,
  .loudThreshold = 600,
  .yellThreshold = 1500,
  .talkPool      = { nullptr },
  .loudPool      = { nullptr },
  .yellPool      = { nullptr }
};


// ============================================================================
// MASTER FACE SELECTOR LIST
// ============================================================================

// Add, remove, or reorder faces here. `numFaces` updates automatically!
const Face faceSelector[] = {
  normalFace,
  smugFace,
  normalSilentFace
};

// Total number of faces in `faceSelector` (computed at compile time)
const uint8_t numFaces = sizeof(faceSelector) / sizeof(faceSelector[0]);
