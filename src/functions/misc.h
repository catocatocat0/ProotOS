/* ============================================================================
 * MISCELLANEOUS MATH & SIGNAL FILTERING FUNCTIONS
 * ============================================================================
 * Defines helper utilities (primarily math and signal processing).
 * ============================================================================ */

#pragma once

// ============================================================================
// ROLLING AVERAGE CONFIGURATION & STATE
// ============================================================================

const int avgWindow = 2;
int avgIndex        = 0;
int avgSum          = 0;
int avgBuffer[avgWindow];


// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

int rollingAverage(int val);


// ============================================================================
// FUNCTION IMPLEMENTATIONS
// ============================================================================

/**
 * Computes a rolling average of the input value over `avgWindow` samples.
 * Acts as a simple low-pass filter to smooth out sudden loud spikes or
 * background chatter from the microphone signal.
 *
 * @param val  The new sample value to include in the rolling average
 * @return     The averaged value across the current window
 */
int rollingAverage(int val) {
  avgSum = avgSum - avgBuffer[avgIndex]; // Remove the oldest entry from the sum
  avgBuffer[avgIndex] = val;             // Add the newest reading to the window
  avgSum = avgSum + val;                 // Add the newest reading to the sum
  avgIndex = (avgIndex + 1) % avgWindow; // Advance index and wrap around window size

  return avgSum / avgWindow;             // Return the average across the window
}
