/* ============================================================================
 * FACIAL EXPRESSION STRUCT & SELECTOR POOLS
 * ============================================================================
 * Defines the `Face` struct and all configured facial expressions for the
 * Protogen. Each `Face` bundles its idle sprite, blink sprite, visual effect
 * modifier, frame timing, microphone thresholds, and speaking sprite pools.
 * ============================================================================ */

#include <Arduino.h>
#pragma once

#include "../functions/faces_struct.h"


// ============================================================================
// INDIVIDUAL FACE CONFIGURATIONS
// ============================================================================

// ----------------------------------------------------------------------------
// STEPS TO DEFINING A CUSTOM FACE
// ----------------------------------------------------------------------------
// 1. Run convimg.py to generate a static sprite asset from a PNG file.
// This step automatically updates and puts the sprite into the code base. It will have the same variable name as its filename
// 2. Copy and paste the CUSTOM FACE TEMPLATE down below and fill out the form
// 3. Add the new face to the `faceSelector[]` array at the bottom of this file. Make sure it is defined as a pointer (e.g. `&faceNewFace`).

// ============================================================================
// CUSTOM FACE TEMPLATE
// ============================================================================
/* 
const Face ~Name of the face variable~ = {
  .idleSprite    = ~Sprite to play when not talking (idle)~,
  .blinkSprite   = ~Sprite to play when not blinking (idle)~,
  .blinkHoldTime = ~Duration in ms to hold `blinkSprite` in `holdFace()`~,
  .offsetX       = `~Horizontal pixel offset (`offset_x`)~,
  .offsetY       = `~Vertical pixel offset (`offset_y`)~,
  .overlay       = `TODO, doesn't work currently, supposed to let you overlay on the prev frame`,
  .mirror        = `~Horizontal panel mirroring toggle (`mirror`)~`,
  .modifier      = `~Post-processing effect (`modifier`; nullptr disables effect)~`,
  .canTalk       = `~Whether this face can talk (`canTalk`)~`,
  .faceDelay     = `~Frame delay in ms for this face (`faceDelay`)~`,
  .talkThreshold = `~Microphone volume threshold for normal talking (`talkThreshold`)~`,
  .loudThreshold = `~Microphone volume threshold for loud talking (`loudThreshold`)~`,
  .yellThreshold = `~Microphone volume threshold for yelling (`yellThreshold`)~`,
  .talkPool      = { ~Array of normal-talking sprites (`talkPool`) Define as an array of sprite pointers 
  example: {talk1, talk2, talk3, talk4, talk5}~ },
  .loudPool      = { ~Array of loud-talking sprites (`loudPool`) Define as an array of sprite pointers 
  .yellPool      = { ~Array of yelling sprites (`yellPool`) Define as an array of sprite pointers 
};
*/

// Standard expression
const Face normalFace = {
  .idleSprite    = normal,
  .blinkSprite   = blink,
  .blinkHoldTime = 100,
  .offsetX       = 0,
  .offsetY       = 0,
  .overlay       = true,
  .mirror        = true,
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

// Smug expression
const Face smugFace = {
  .idleSprite    = smug,
  .blinkSprite   = blink,
  .blinkHoldTime = 100,
  .offsetX       = 0,
  .offsetY       = 0,
  .overlay       = true,
  .mirror        = true,
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
  .blinkHoldTime = 100,
  .offsetX       = 0,
  .offsetY       = 0,
  .overlay       = true,
  .mirror        = true,
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

// Add, remove, or reorder `Face` pointers here. Any trailing unfilled slots
// up to `MAX_FACES` automatically default to `nullptr`.
const Face* faceSelector[MAX_FACES] = {
  &normalFace,
  &smugFace,
  &normalSilentFace
};
