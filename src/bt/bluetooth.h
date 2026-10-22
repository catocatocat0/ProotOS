/* ============================================================================
 * BLUETOOTH SERIAL MODULE (WIP)
 * ============================================================================
 * Handles Bluetooth Classic SPP pairing (via Simple Secure Pairing / boop
 * confirmation) and incoming command packet parsing.
 * ============================================================================ */

#include "BluetoothSerial.h"

// ============================================================================
// HARDWARE & BUFFER CONFIGURATION
// ============================================================================

#define BOOP_PIN    36   // Detects booping to confirm Bluetooth pairing (positive sensor output relative to GND)
#define BUFFER_SIZE 1024 // Size of the incoming Bluetooth packet buffer


// ============================================================================
// BLUETOOTH STATE & BUFFER VARIABLES
// ============================================================================

BluetoothSerial SerialBT;

int     BUFFER_INDEX = 0;
uint8_t BUFFER_ARRAY[BUFFER_SIZE];

boolean confirmRequestPending = false; // True while waiting for SSP pairing confirmation
bool    btConnectionStatus    = false; // True if a client is currently connected
bool    btStatus              = false; // True if the Bluetooth service initialized successfully


// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

void BTAuthCompleteCallback(boolean success);
void BTConfirmRequestCallback(uint32_t numVal);
void btWrite(const char* value);
void setupBluetooth();
uint8_t btServerLoop();


// ============================================================================
// SSP PAIRING CALLBACKS & HELPERS
// ============================================================================

/**
 * Callback triggered when Simple Secure Pairing (SSP) authentication finishes.
 */
void BTAuthCompleteCallback(boolean success) {
  confirmRequestPending = false;
  if (success) {
    Serial.println("Pairing success.");
  } else {
    Serial.println("Pairing failed, rejected by user.");
  }
}

/**
 * Callback triggered when a device requests SSP pairing confirmation.
 */
void BTConfirmRequestCallback(uint32_t numVal) {
  confirmRequestPending = true;
  Serial.println(numVal);
}

/**
 * Sends a null-terminated string over Bluetooth Serial.
 */
void btWrite(const char* value) {
  SerialBT.write((const uint8_t*)value, strlen(value));
}


// ============================================================================
// BLUETOOTH INITIALIZATION & MAIN LOOP
// ============================================================================

/**
 * Initializes the Bluetooth Serial service with Simple Secure Pairing (SSP).
 */
void setupBluetooth() {
  // Configure boop sensor pin used to confirm pairing requests
  pinMode(BOOP_PIN, INPUT);

  // Enable SSP and register pairing callbacks
  SerialBT.enableSSP();
  SerialBT.onConfirmRequest(BTConfirmRequestCallback);
  SerialBT.onAuthComplete(BTAuthCompleteCallback);

  // Start Bluetooth service with device name
  String devName("KATXE PROTOGEN");
  btStatus = SerialBT.begin(devName);

  if (SerialBT.available()) {
    Serial.println("Bluetooth ON");
  } else {
    Serial.println("Bluetooth OFF");
  }

  btConnectionStatus = false;
}

/**
 * Processes Bluetooth pairing requests and incoming data packets.
 *
 * @return Status code for `main.cpp`:
 *         - `0`: Normal operation (no pending Bluetooth action)
 *         - `1`: Pairing request pending (display Bluetooth confirmation screen)
 */
uint8_t btServerLoop() {
  // Exit immediately if Bluetooth is not enabled
  if (!btStatus) {
    return;
  }

  // Handle Pending SSP Pairing Confirmation
  if (confirmRequestPending) {
    if (digitalRead(BOOP_PIN) == HIGH) {
      SerialBT.confirmReply(true);
    }
    delay(10);
    return 1;
  }

  // Update Client Connection Status
  if (btConnectionStatus != SerialBT.hasClient()) {
    btConnectionStatus = !btConnectionStatus;
  }

  // Exit if no client is connected or no data is waiting
  if (!btConnectionStatus || !SerialBT.available()) {
    return 0;
  }

  // Read Incoming Packet into Buffer
  BUFFER_INDEX = 0;
  while (SerialBT.available()) {
    int data = SerialBT.read();
    BUFFER_ARRAY[BUFFER_INDEX++] = (byte)data;
  }

  // Unpack Packet Header
  // Bytes 0–1: Command identifier
  // Bytes 2–3: Metadata (e.g., payload length)
  // Bytes 4+:  Payload data
  uint16_t btCmd      = BUFFER_ARRAY[0] >> 8 & 0xFF | BUFFER_ARRAY[1];
  uint16_t btMetaData = BUFFER_ARRAY[2] >> 8 & 0xFF | BUFFER_ARRAY[3];
}