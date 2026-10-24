/* ============================================================================
 * HELMET INTERNAL SSD1306 128x32 I2C OLED DISPLAY DRIVER (CORE 0)
 * ============================================================================
 * Zero-dependency, lightweight SSD1306 128x32 I2C OLED driver designed to
 * mirror the current 128x32 HUB75 LED matrix frame inside the helmet.
 *
 * Features:
 * - Probes the I2C bus at boot (`0x3C`); if no OLED is connected, the Core 0
 *   task is never spawned and `drawFace()` / `playAnimation()` skip OLED writes
 *   for zero runtime overhead.
 * - Runs on Core 0 (`PRO_CPU_NUM` = 0) at a throttled refresh rate (~10 FPS /
 *   100ms interval) with `vTaskDelay` so Core 0 has plenty of CPU headroom for
 *   Bluetooth and system tasks.
 * - Uses only 512 bytes of RAM for the 128x32 1-bit page buffer and streams
 *   pages over 400 kHz Fast-Mode I2C.
 * ============================================================================ */

#pragma once

#include <Arduino.h>
#include <Wire.h>

// ============================================================================
// OLED HARDWARE & TASK CONFIGURATION
// ============================================================================

#define OLED_SDA_PIN     21   // ESP32 default hardware I2C SDA pin
#define OLED_SCK_PIN     22   // ESP32 default hardware I2C SCL/SCK pin
#define OLED_I2C_ADDR    0x3C // Standard I2C address for 0.91" 128x32 SSD1306 OLEDs
#define OLED_I2C_CLOCK   400000UL // 400 kHz Fast-Mode I2C

#define OLED_WIDTH       128
#define OLED_HEIGHT      32
#define OLED_PAGES       (OLED_HEIGHT / 8)
#define OLED_BUF_SIZE    (OLED_WIDTH * OLED_PAGES) // 512 bytes total

#define OLED_REFRESH_MS  100  // 10 FPS refresh interval on Core 0 (saves CPU for Bluetooth)
#define OLED_CORE_ID     0    // Run on Core 0 (Arduino `loop()` runs on Core 1)


// ============================================================================
// OLED STATE & FRAME BUFFER
// ============================================================================

bool         oledConnected = false;           // Set to true at boot only if 0x3C ACKs on I2C
volatile bool oledFrameDirty = false;         // Signals Core 0 that a new matrix frame was drawn
uint8_t      oledBackBuffer[OLED_BUF_SIZE];   // Written by Core 1 during matrix rendering
uint8_t      oledTxBuffer[OLED_BUF_SIZE];     // Sent over I2C by Core 0 task
TaskHandle_t oledTaskHandle = nullptr;


// ============================================================================
// LOW-LEVEL SSD1306 I2C HELPERS
// ============================================================================

/**
 * Sends a single command byte to the SSD1306 controller over I2C.
 */
inline void oledSendCommand(uint8_t cmd) {
  Wire.beginTransmission(OLED_I2C_ADDR);
  Wire.write(0x00); // Co = 0, D/C# = 0 (command stream)
  Wire.write(cmd);
  Wire.endTransmission();
}

/**
 * Sends a list of initialization commands to configure the 128x32 SSD1306 panel.
 */
inline void oledInitController() {
  const uint8_t initSeq[] = {
    0xAE,       // Display OFF
    0xD5, 0x80, // Set display clock divide ratio / oscillator frequency
    0xA8, 0x1F, // Set multiplex ratio to 32-1 (0x1F for 128x32)
    0xD3, 0x00, // Set display offset = 0
    0x40,       // Set display start line = 0
    0x8D, 0x14, // Enable internal charge pump
    0x20, 0x00, // Set memory addressing mode = Horizontal Addressing Mode
    0xA1,       // Segment re-map (column 127 mapped to SEG0)
    0xC8,       // COM output scan direction remapped
    0xDA, 0x02, // Set COM pins hardware configuration (0x02 for 128x32)
    0x81, 0x7F, // Set contrast control (medium-high brightness)
    0xD9, 0xF1, // Set pre-charge period
    0xDB, 0x40, // Set VCOMH deselect level
    0xA4,       // Entire display ON (resume to RAM content)
    0xA6,       // Normal display (non-inverted)
    0x2E,       // Deactivate scrolling
    0xAF        // Display ON
  };

  for (size_t i = 0; i < sizeof(initSeq); i++) {
    oledSendCommand(initSeq[i]);
  }
}

/**
 * Streams the 512-byte `oledTxBuffer` to the SSD1306 in 64-byte I2C bursts.
 */
inline void oledFlushBuffer() {
  // Set column address range (0..127) and page address range (0..3)
  Wire.beginTransmission(OLED_I2C_ADDR);
  Wire.write(0x00);
  Wire.write(0x21);
  Wire.write(0);
  Wire.write(OLED_WIDTH - 1);
  Wire.write(0x22);
  Wire.write(0);
  Wire.write(OLED_PAGES - 1);
  Wire.endTransmission();

  // Transmit 512 bytes ofGDDRAM in 64-byte chunks (fits comfortably in ESP32 Wire 128-byte FIFO)
  const uint16_t chunkSize = 64;
  for (uint16_t offset = 0; offset < OLED_BUF_SIZE; offset += chunkSize) {
    Wire.beginTransmission(OLED_I2C_ADDR);
    Wire.write(0x40); // Co = 0, D/C# = 1 (GDDRAM data stream)
    Wire.write(&oledTxBuffer[offset], chunkSize);
    Wire.endTransmission();
  }
}


// ============================================================================
// CORE 1 PIXEL MIRRORING HELPERS (CALLED FROM DRAWFACES.H)
// ============================================================================

/**
 * Clears the 512-byte OLED back buffer. Does nothing if no OLED was detected at boot.
 */
inline void oledClearBuffer() {
  if (!oledConnected) {
    return;
  }
  memset(oledBackBuffer, 0, OLED_BUF_SIZE);
}

/**
 * Plots or clears a single pixel in the 128x32 OLED back buffer based on a
 * 16-bit RGB565 matrix pixel color (any non-black pixel lights up the OLED pixel).
 */
inline void oledDrawPixel(int x, int y, uint16_t rgb565) {
  if (!oledConnected) {
    return;
  }
  if ((unsigned int)x >= OLED_WIDTH || (unsigned int)y >= OLED_HEIGHT) {
    return;
  }

  uint16_t index = x + ((y >> 3) << 7); // x + (y / 8) * 128
  uint8_t  mask  = (1 << (y & 7));

  if (rgb565 != 0) {
    oledBackBuffer[index] |= mask;
  } else {
    oledBackBuffer[index] &= ~mask;
  }
}

/**
 * Marks the current OLED back buffer as ready to be pushed by the Core 0 task.
 */
inline void oledCommitFrame() {
  if (!oledConnected) {
    return;
  }
  oledFrameDirty = true;
}


// ============================================================================
// CORE 0 BACKGROUND TASK & BOOT INITIALIZATION
// ============================================================================

/**
 * FreeRTOS task pinned to Core 0 (`OLED_CORE_ID`).
 * Wakes up every `OLED_REFRESH_MS` (100ms / 10 FPS) and pushes the frame buffer
 * over I2C only when a new face/animation frame has been committed.
 */
inline void oledCore0Task(void* parameter) {
  const TickType_t delayTicks = pdMS_TO_TICKS(OLED_REFRESH_MS);

  for (;;) {
    if (oledFrameDirty) {
      oledFrameDirty = false;
      memcpy(oledTxBuffer, oledBackBuffer, OLED_BUF_SIZE);
      oledFlushBuffer();
    }
    vTaskDelay(delayTicks);
  }
}

/**
 * Probes the I2C bus (`OLED_SDA_PIN` = 21, `OLED_SCK_PIN` = 22) for an SSD1306
 * display at `0x3C`. If found, initializes the OLED and spawns the Core 0
 * background refresh task. If not found, disables all OLED logic to save CPU.
 */
inline void setupOLED() {
  Wire.begin(OLED_SDA_PIN, OLED_SCK_PIN, OLED_I2C_CLOCK);

  // Check if an I2C device acknowledges at 0x3C
  Wire.beginTransmission(OLED_I2C_ADDR);
  uint8_t error = Wire.endTransmission();

  if (error != 0) {
    oledConnected = false;
    Serial.println("[OLED] No SSD1306 detected on I2C (0x3C). Core 0 OLED task disabled.");
    return;
  }

  oledConnected = true;
  Serial.println("[OLED] SSD1306 128x32 detected on I2C (0x3C). Starting Core 0 task...");

  memset(oledBackBuffer, 0, OLED_BUF_SIZE);
  memset(oledTxBuffer, 0, OLED_BUF_SIZE);

  oledInitController();
  oledFlushBuffer();

  // Spawn background refresh task on Core 0 at low priority (1) so Bluetooth / Wi-Fi take precedence
  xTaskCreatePinnedToCore(
    oledCore0Task,   // Task function
    "OLED_Core0",    // Task name
    2048,            // Stack size (bytes)
    nullptr,         // Task parameter
    1,               // Low priority (yields to BT controller on Core 0)
    &oledTaskHandle, // Task handle
    OLED_CORE_ID     // Pinned to Core 0
  );
}

