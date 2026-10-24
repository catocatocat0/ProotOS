/* ============================================================================
 * FACIAL EXPRESSION STRUCT & SELECTOR POOLS
 * ============================================================================
 * Defines the `Face` struct and all configured facial expressions for the
 * Protogen. Each `Face` bundles its idle sprite, blink sprite, visual effect
 * modifier, frame timing, microphone thresholds, and speaking sprite pools.
 * ============================================================================ */

#include <Arduino.h>
#pragma once

#include "../functions/boops_struct.h"


// ============================================================================
// INDIVIDUAL BOOP CONFIGURATIONS
// ============================================================================

// ----------------------------------------------------------------------------
// STEPS TO DEFINING A CUSTOM BOOP ANIMATION
// ----------------------------------------------------------------------------
// 1. Run either convgif.py or convimg.py to generate either an animation or static sprite asset from a GIF or PNG file.
// This step automatically updates and puts the animation/sprite into the code base. It will have the same variable name as its filename
// 2. Copy and paste the CUSTOM BOOP TEMPLATE down below and fill out the form
// 3. Add the new boop to the `boopPool[]` array at the bottom of this file. Make sure it is defined as a pointer (e.g. `&boopNewBoop`).

// ----------------------------------------------------------------------------
// CUSTOM BOOP TEMPLATE
// ----------------------------------------------------------------------------
/* 
const Boop ~Name of the boop variable~ = {
  .animation    = ~Pointer to the animation sequence (`animation`), make sure to define as &`variable`.
  Set to nullptr if not used~,
  .staticSprite = ~Pointer to the static sprite (`staticSprite`), make sure to define as `variable`.
  Set to nullptr if not used~,
  .holdTime     = ~Duration in ms for `holdFace()` (if <= 0, draws a single frame via `drawFace()`)~,
  .offsetX      = ~Horizontal pixel offset (`offset_x`)~,
  .offsetY      = ~Vertical pixel offset (`offset_y`)~,
  .overlay      = ~Frame buffer overlay toggle (`overlay`)~,
  .mirror       = TODO, doesn't work currently, supposed to let you overlay on the prev frame,
  .modifier     = ~Post-processing effect (`modifier`; nullptr disables effect)~,
  .poolWeight   = ~How many times this boop animation appears in the selection pool (`poolWeight`)~
};
*/


const Boop boopBk = {
  .animation    = &bk,
  .staticSprite = nullptr,
  .holdTime     = 0,
  .offsetX      = 0,
  .offsetY      = 0,
  .overlay      = true,
  .mirror       = true,
  .modifier     = nullptr,
  .poolWeight   = 1
};

const Boop boopBk1 = {
  .animation    = &bk1_,
  .staticSprite = nullptr,
  .holdTime     = 0,
  .offsetX      = 0,
  .offsetY      = 0,
  .overlay      = false,
  .mirror       = true,
  .modifier     = nullptr,
  .poolWeight   = 1
};

const Boop boopBk2 = {
  .animation    = &bk2_,
  .staticSprite = nullptr,
  .holdTime     = 0,
  .offsetX      = 0,
  .offsetY      = 0,
  .overlay      = true,
  .mirror       = true,
  .modifier     = nullptr,
  .poolWeight   = 1
};

const Boop boopAmogus = {
  .animation    = nullptr,
  .staticSprite = amogus,
  .holdTime     = 2500,
  .offsetX      = 0,
  .offsetY      = 0,
  .overlay      = false,
  .mirror       = true,
  .modifier     = abberation,
  .poolWeight   = 1
};

const Boop boopBsod = {
  .animation    = nullptr,
  .staticSprite = bsod,
  .holdTime     = 2500,
  .offsetX      = 0,
  .offsetY      = 0,
  .overlay      = false,
  .mirror       = true,
  .modifier     = nullptr,
  .poolWeight   = 1
};


// ============================================================================
// MASTER BOOP SELECTION POOL
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
