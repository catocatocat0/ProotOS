/* ============================================================================
 * PROOTOS - ESP32 PROTOGEN MATRIX CONTROLLER
 * ============================================================================
 * NOTE BEFORE USING:
 * - Use VSCode and PlatformIO to flash the ESP32. Make sure to change
 *   platformio.ini configs to match the specific board you are using.
 * - Tested on a FireBeetle ESP32-E and a standard ESP32 devboard.
 * - Reference wiring guide: https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA
 *   (Note: This code uses a modified wiring scheme—check pin mappings in drawfaces.h!)
 *
 * Based on: https://github.com/PanickingLynx/ProtOpenSource
 * Author:   Katxe
 * Telegram: @Katxe
 * Bsky:     @katxe.bsky.social
 * ============================================================================ */

// ============================================================================
// DISPLAY & PROJECT DEPENDENCIES
// ============================================================================

// Uncomment this if your matrices display errors like color shifts or flickering pixels
// (Common with matrices bought from AliExpress)
#define PxMATRIX_SPI_FREQUENCY 19900000

// Enables double buffering for seamless image transitions on the matrix
#define double_buffer true

#include <Arduino.h>
#include "functions/misc.h"
#include "functions/drawfaces.h"
#include "functions/faces.h"

// Bluetooth module (WIP - uncomment when reimplementing Bluetooth)
// #include "bt/bluetooth.h"


// ============================================================================
// HARDWARE PIN DEFINITIONS
// ============================================================================

#define MIC_PIN       35  // Connect the OUT pin of the MAX4466 microphone here
#define BOOP_PIN      33  // Detects booping & confirms BT pairing (connect sensor positive output relative to GND)
#define FACE_SELECTOR 32  // Cycles through protogen emotions (wire a button connected to 3.3V)


// ============================================================================
// MICROPHONE & TIMING CONFIGURATION
// ============================================================================

// Baseline analog reading from the microphone in a quiet environment.
// To calibrate: temporarily set this to 0, print `microphoneLevel` to Serial,
// and observe the resting value on the Serial Plotter with no sound.
const int baseline = 1945;

// Default frame delay (in ms) used when holding static sprites in `holdFace()`
const int defaultFaceDelay = 100;


// ============================================================================
// STATE & TIMING VARIABLES
// ============================================================================

unsigned long previousMillis;
unsigned long blinkMillis;
unsigned long currentMillis = millis();
unsigned long prevButton    = 0; // Used to debounce the face selector button

uint8_t currFace   = 1;                // Currently active face index in `faceSelector[]` (1-indexed)
Face    activeFace = faceSelector[0];  // Cached active Face struct (only updated when `selectFaceISR` triggers)
bool    booped     = false;            // Flag set by boopISR when the boop sensor triggers


// ============================================================================
// FUNCTION PREDEFINES
// ============================================================================

void isSpeaking(const uint32_t* speakSprite);
void isIdle();
void holdFace(int time, int offset_x, int offset_y, const uint32_t image[], bool overlay, bool mirror, FaceModifier* modifier);
void IRAM_ATTR boopISR();
void IRAM_ATTR selectFaceISR();


// ============================================================================
// SETUP & MAIN LOOP
// ============================================================================

void setup() {
  // Initialize Bluetooth (WIP)
  // setupBluetooth();

  // Initialize the initial face's sprite pool counts once at startup
  activeFace.initCounts();

  // Initialize the LED matrix display and boot screen
  startUP();

  // Configure microphone input
  pinMode(MIC_PIN, INPUT);

  // Configure hardware interrupts for boop sensor and face selector button
  pinMode(BOOP_PIN, INPUT_PULLDOWN);
  attachInterrupt(BOOP_PIN, boopISR, RISING);

  pinMode(FACE_SELECTOR, INPUT_PULLDOWN);
  attachInterrupt(FACE_SELECTOR, selectFaceISR, FALLING);
}

void loop() {
  // Handle Boop Sensor Trigger
  if (booped) {
    // Reset flag immediately (and after animations) to prevent buffering multiple boops
    booped = false;

    // Pick a random boop reaction animation.
    // NOTE: Increase the upper bound of random() if you add more cases.
    int rand = random(0, 4);
    switch (rand) {
      case 0:
        playAnimation(0, 0, bk, true, true);
        booped = false;
        break;

      case 1:
        playAnimation(0, 0, bk1_, false, true);
        booped = false;
        break;

      case 2:
        playAnimation(0, 0, bk2_, true, true);
        booped = false;
        break;

      case 3:
        holdFace(2500, 0, 0, amogus, false, true, abberation);
        booped = false;
        break;

      case 4:
        holdFace(2500, 0, 0, bsod, false, true, abberation);
        booped = false;
        break;
    }
  }

  // Read & Filter Microphone Input
  // Subtract baseline to center the waveform, take the absolute amplitude for loudness,
  // and pass it through a rolling average (low-pass filter) to smooth spikes.
  int microphoneLevel = rollingAverage(abs(analogRead(MIC_PIN) - baseline));

  // Update Facial Animation (Speaking vs. Idle) using cached `activeFace`
  if (activeFace.canTalk && microphoneLevel > activeFace.talkThreshold) {
    const uint32_t* speakSprite = nullptr;

    // Select speaking animation tier from loudest to quietest
    if (microphoneLevel > activeFace.yellThreshold) {
      speakSprite = activeFace.yell();
    } else if (microphoneLevel > activeFace.loudThreshold) {
      speakSprite = activeFace.loud();
    } else {
      speakSprite = activeFace.talk();
    }

    isSpeaking(speakSprite);
  } else {
    isIdle();
  }

  /* Bluetooth Loop (WIP - TODO: Add in Bluetooth)
  uint8_t btMessage = btServerLoop();
  switch (btMessage) {
    case 0: // Run normally
      if (!boopSensor()) {
        getMicrophoneLevel();
      }
      break;

    case 1: // Play Bluetooth display image
      break;
  }
  */
}


// ============================================================================
// DISPLAY & ANIMATION HELPER FUNCTIONS
// ============================================================================

/**
 * Plays a speaking animation frame for a short random duration (10–200ms),
 * interjecting the active face's blink animation every 5000–10000ms (if `blinkSprite` is set).
 *
 * @param speakSprite  Pointer to the RLE sprite selected from the face's talk/loud/yell pool
 */
void isSpeaking(const uint32_t* speakSprite) {
  previousMillis = currentMillis;

  while (currentMillis - previousMillis <= random(10, 200)) {
    if (activeFace.blinkSprite != nullptr && currentMillis - blinkMillis >= random(5000, 10000)) {
      holdFace(100, 0, 0, activeFace.blinkSprite, true, true, activeFace.modifier);
      blinkMillis = currentMillis;
    } else {
      drawFace(0, 0, speakSprite, true, true, activeFace.modifier);
    }

    delay(activeFace.faceDelay);
    currentMillis = millis();
  }
}

/**
 * Plays the currently active idle face for a short random duration (10–200ms),
 * interjecting the face's blink animation every 2000–10000ms (if `blinkSprite` is set).
 */
void isIdle() {
  previousMillis = currentMillis;

  while (currentMillis - previousMillis <= random(10, 200)) {
    if (activeFace.blinkSprite != nullptr && currentMillis - blinkMillis >= random(2000, 10000)) {
      holdFace(100, 0, 0, activeFace.blinkSprite, true, true, activeFace.modifier);
      blinkMillis = currentMillis;
    } else {
      drawFace(0, 0, activeFace.idleSprite, true, true, activeFace.modifier);
    }

    delay(activeFace.faceDelay);
    currentMillis = millis();
  }
}

/**
 * Holds a static RLE-encoded sprite (generated by convimg.py) on the screen
 * for a specified duration.
 *
 * @param time      Duration to hold the image (in milliseconds)
 * @param offset_x  Horizontal pixel offset
 * @param offset_y  Vertical pixel offset
 * @param image     RLE-encoded sprite array (first 3 entries: width, height, array size;
 *                  subsequent entries: 3 bytes RGB + 1 byte repeat count)
 * @param overlay   If false, clears the display before drawing (allows layering when true)
 * @param mirror    If true, mirrors the sprite across the second panel
 * @param modifier  Optional post-processing effect (e.g., aberration, CRT)
 */
void holdFace(int time, int offset_x, int offset_y, const uint32_t image[], bool overlay, bool mirror, FaceModifier* modifier) {
  previousMillis = currentMillis;

  while (currentMillis - previousMillis <= time) {
    drawFace(offset_x, offset_y, image, overlay, mirror, modifier);
    delay(defaultFaceDelay);
    currentMillis = millis();
  }
}


// ============================================================================
// INTERRUPT SERVICE ROUTINES (ISRs)
// ============================================================================

/**
 * Triggered when BOOP_PIN goes HIGH. Flags the main loop to play a boop animation.
 */
void IRAM_ATTR boopISR() {
  booped = true;
}

/**
 * Triggered when FACE_SELECTOR button is pressed. Cycles through `faceSelector[]`
 * with a 250ms debounce window, updates `activeFace`, and recalculates its pool counts.
 */
void IRAM_ATTR selectFaceISR() {
  if (millis() - prevButton > 250) {
    if (currFace >= numFaces) {
      currFace = 1;
    } else {
      currFace++;
    }

    activeFace = faceSelector[currFace - 1];
    activeFace.initCounts();

    prevButton = millis();
  }
}