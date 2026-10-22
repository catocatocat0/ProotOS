/* ============================================================================
 * BOOP REACTION DEFINITIONS & SELECTION POOL
 * ============================================================================
 * Defines the `Boop` struct, individual boop reaction configurations (both
 * multi-frame GIF animations and static held sprites), and the non-repeating
 * selection pool (`boopPool`).
 * ============================================================================ */

#pragma once

#include <Arduino.h>
#include "drawfaces.h"

// ============================================================================
// BOOP POOL CONFIGURATION
// ============================================================================

// Maximum number of boop reactions that can be registered in `boopPool`.
// Increase this value if you add more than 255 boop reactions.
#define MAX_BOOP_ANIMATIONS 255


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
  int           holdTime;   // Duration in ms for `holdFace()` (if <= 0, draws a single frame via `drawFace()`)
  int           offsetX;    // Horizontal pixel offset (`offset_x`)
  int           offsetY;    // Vertical pixel offset (`offset_y`)
  bool          overlay;    // Frame buffer overlay toggle (`overlay`)
  bool          mirror;     // Horizontal panel mirroring toggle (`mirror`)
  FaceModifier* modifier;   // Post-processing effect (`modifier`; nullptr disables effect)
  uint8_t       poolWeight; // Number of times this boop can be picked before being removed from the pool cycle

  /**
   * Plays the boop reaction on the display using all configured parameters.
   * If both `animation` and `staticSprite` are set, it plays them sequentially
   * (GIF animation first, followed by the held static sprite).
   * Fully nullptr-safe if neither asset pointer (or internal animation data) is set.
   */
  inline void play() const {
    if (animation != nullptr && animation->animation != nullptr) {
      playAnimation(offsetX, offsetY, *animation, overlay, mirror);
    }
    if (staticSprite != nullptr) {
      if (holdTime > 0) {
        holdFace(holdTime, offsetX, offsetY, staticSprite, overlay, mirror, modifier);
      } else {
        drawFace(offsetX, offsetY, staticSprite, overlay, mirror, modifier);
      }
    }
  }
};


// ============================================================================
// BOOP SELECTION POOL STATE & HELPERS
// ============================================================================

// Defined by the user in `src/user/boops.h`
extern const Boop* boopPool[MAX_BOOP_ANIMATIONS];

uint8_t numBoopAnimations = 0; // Total non-null boops in `boopPool` (set by `initBoopPool()`)
uint8_t boopPoolRemaining = 0; // Remaining unexhausted boops in the current cycle
uint8_t boopRemainingUses[MAX_BOOP_ANIMATIONS]; // Remaining picks for each boop before removal

/**
 * Resets the active pool and restores each boop's remaining uses to its `poolWeight`.
 */
inline void resetBoopPool() {
  boopPoolRemaining = 0;
  for (uint8_t i = 0; i < numBoopAnimations; i++) {
    if (boopPool[i]->poolWeight > 0) {
      // Ensure active boops with poolWeight > 0 are placed at the front of the active pool
      const Boop* temp             = boopPool[boopPoolRemaining];
      boopPool[boopPoolRemaining]  = boopPool[i];
      boopPool[i]                  = temp;
      boopRemainingUses[boopPoolRemaining] = boopPool[boopPoolRemaining]->poolWeight;
      boopPoolRemaining++;
    }
  }
}

/**
 * Counts the non-null `Boop` pointers in `boopPool[]` until it encounters
 * `nullptr` (or reaches `MAX_BOOP_ANIMATIONS`) and initializes the pool bounds
 * and weights. Called once during `setup()`.
 */
inline void initBoopPool() {
  numBoopAnimations = 0;
  while (numBoopAnimations < MAX_BOOP_ANIMATIONS && boopPool[numBoopAnimations] != nullptr) {
    numBoopAnimations++;
  }
  resetBoopPool();
}

/**
 * Picks a random `Boop` from the active pool and decrements its remaining uses.
 * Once a boop's remaining uses reach 0, it is swapped to the back to remove it
 * from the active pool. When all boops are exhausted, the pool resets automatically.
 */
inline const Boop* pickNextBoop() {
  if (numBoopAnimations == 0) {
    return nullptr;
  }

  if (boopPoolRemaining == 0) {
    resetBoopPool();
    if (boopPoolRemaining == 0) {
      return nullptr;
    }
  }

  uint8_t     pickIndex = random(0, boopPoolRemaining);
  const Boop* chosen    = boopPool[pickIndex];

  // Decrement remaining uses for this boop; only remove it from the pool when it hits 0
  boopRemainingUses[pickIndex]--;
  if (boopRemainingUses[pickIndex] == 0) {
    boopPoolRemaining--;
    boopPool[pickIndex]          = boopPool[boopPoolRemaining];
    boopPool[boopPoolRemaining]  = chosen;
    boopRemainingUses[pickIndex] = boopRemainingUses[boopPoolRemaining];
  }

  return chosen;
}

