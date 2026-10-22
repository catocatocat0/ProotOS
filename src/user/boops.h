/* ============================================================================
 * BOOP REACTION DEFINITIONS & SELECTION POOL
 * ============================================================================
 * Defines the `Boop` struct, individual boop reaction configurations (both
 * multi-frame GIF animations and static held sprites), and the non-repeating
 * selection pool (`boopPool`).
 * ============================================================================ */

#pragma once

#include <Arduino.h>
#include "../functions/drawfaces.h"

// ============================================================================
// BOOP POOL CONFIGURATION
// ============================================================================

// Maximum number of boop reactions that can be registered in `boopPool`.
// Increase this value if you add more than 10 boop reactions.
#define MAX_BOOP_ANIMATIONS 10


// ============================================================================
// BOOP STRUCT DEFINITION
// ============================================================================

/**
 * Represents a single boop reaction animation or static held face.
 *
 * - Set `animation` (e.g., `&bk`) for a multi-frame GIF animation and leave
 *   `staticSprite` as `nullptr`.
 * - Set `staticSprite` (e.g., `amogus`) for a static image held for `holdTime`
 *   milliseconds and leave `animation` as `nullptr`.
 * - Calling `.play()` automatically detects which asset type is configured
 *   and invokes `playAnimation()` or `holdFace()` accordingly.
 */
struct Boop {
  // Asset pointers (set one and leave the other as `nullptr`)
  const Animation* animation;    // GIF animation for `playAnimation()`
  const uint32_t*  staticSprite; // Static RLE sprite for `holdFace()` / `drawFace()`

  // Display & timing parameters for `playAnimation()`, `holdFace()`, and `drawFace()`
  int           holdTime; // Duration in ms for `holdFace()` (if <= 0, draws a single frame via `drawFace()`)
  int           offsetX;  // Horizontal pixel offset (`offset_x`)
  int           offsetY;  // Vertical pixel offset (`offset_y`)
  bool          overlay;  // Frame buffer overlay toggle (`overlay`)
  bool          mirror;   // Horizontal panel mirroring toggle (`mirror`)
  FaceModifier* modifier; // Post-processing effect (`modifier`; nullptr disables effect)

  /**
   * Automatically determines whether this boop is a GIF Animation or a static
   * sprite and plays it on the display using all configured parameters.
   * Fully nullptr-safe if neither asset pointer (or internal animation data) is set.
   */
  inline void play() const {
    if (animation != nullptr && animation->animation != nullptr) {
      playAnimation(offsetX, offsetY, *animation, overlay, mirror);
    } else if (staticSprite != nullptr) {
      if (holdTime > 0) {
        holdFace(holdTime, offsetX, offsetY, staticSprite, overlay, mirror, modifier);
      } else {
        drawFace(offsetX, offsetY, staticSprite, overlay, mirror, modifier);
      }
    }
  }
};


// ============================================================================
// INDIVIDUAL BOOP CONFIGURATIONS
// ============================================================================

const Boop boopBk = {
  .animation    = &bk,
  .staticSprite = nullptr,
  .holdTime     = 0,
  .offsetX      = 0,
  .offsetY      = 0,
  .overlay      = true,
  .mirror       = true,
  .modifier     = nullptr
};

const Boop boopBk1 = {
  .animation    = &bk1_,
  .staticSprite = nullptr,
  .holdTime     = 0,
  .offsetX      = 0,
  .offsetY      = 0,
  .overlay      = false,
  .mirror       = true,
  .modifier     = nullptr
};

const Boop boopBk2 = {
  .animation    = &bk2_,
  .staticSprite = nullptr,
  .holdTime     = 0,
  .offsetX      = 0,
  .offsetY      = 0,
  .overlay      = true,
  .mirror       = true,
  .modifier     = nullptr
};

const Boop boopAmogus = {
  .animation    = nullptr,
  .staticSprite = amogus,
  .holdTime     = 2500,
  .offsetX      = 0,
  .offsetY      = 0,
  .overlay      = false,
  .mirror       = true,
  .modifier     = abberation
};

const Boop boopBsod = {
  .animation    = nullptr,
  .staticSprite = bsod,
  .holdTime     = 2500,
  .offsetX      = 0,
  .offsetY      = 0,
  .overlay      = false,
  .mirror       = true,
  .modifier     = abberation
};


// ============================================================================
// MASTER BOOP SELECTION POOL & HELPERS
// ============================================================================

// Add or remove `Boop` pointers here. Any trailing unfilled slots up to
// `MAX_BOOP_ANIMATIONS` automatically default to `nullptr`.
const Boop* boopPool[MAX_BOOP_ANIMATIONS] = {
  &boopBk,
  &boopBk1,
  &boopBk2,
  &boopAmogus,
  &boopBsod
};

uint8_t numBoopAnimations = 0; // Total non-null boops in `boopPool` (set by `initBoopPool()`)
uint8_t boopPoolRemaining = 0; // Remaining unplayed boops in the current cycle

/**
 * Counts the non-null `Boop` pointers in `boopPool[]` until it encounters
 * `nullptr` (or reaches `MAX_BOOP_ANIMATIONS`) and initializes the pool bounds.
 * Called once during `setup()`.
 */
inline void initBoopPool() {
  numBoopAnimations = 0;
  while (numBoopAnimations < MAX_BOOP_ANIMATIONS && boopPool[numBoopAnimations] != nullptr) {
    numBoopAnimations++;
  }
  boopPoolRemaining = numBoopAnimations;
}

/**
 * Picks a random unplayed `Boop` from the active pool, swaps it to the back so
 * it won't repeat in the same cycle, and automatically resets the pool once all
 * configured boops have been played.
 */
inline const Boop* pickNextBoop() {
  if (numBoopAnimations == 0) {
    return nullptr;
  }

  if (boopPoolRemaining == 0) {
    boopPoolRemaining = numBoopAnimations;
  }

  uint8_t     pickIndex = random(0, boopPoolRemaining);
  const Boop* chosen    = boopPool[pickIndex];

  boopPoolRemaining--;
  boopPool[pickIndex]         = boopPool[boopPoolRemaining];
  boopPool[boopPoolRemaining] = chosen;

  return chosen;
}

