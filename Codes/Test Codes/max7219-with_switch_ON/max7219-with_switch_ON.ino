#include <MD_MAX72xx.h>
#include <SPI.h>

#define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW

// Pin Definitions
#define DIN_PIN  11
#define CS_PIN   10
#define SWITCH_PIN 2

// Create an instance for MAX7219
MD_MAX72XX mx = MD_MAX72XX(DIN_PIN, CS_PIN, 1);

void setup() {
  pinMode(SWITCH_PIN, INPUT_PULLUP); // Set switch pin as input with pull-up
  Serial.begin(115200); // Start serial communication
  mx.begin(); // Initialize MAX7219
  mx.clear(); // Clear the display
  Serial.println("MAX7219 Initialized");
}

void loop() {
  // Read the switch state
  if (digitalRead(SWITCH_PIN) == LOW) { // Switch is pressed
    Serial.println("Switch Pressed - Turning on all LEDs");
    // Turn on all columns of the matrix
    for (int i = 0; i < 8; i++) {
      mx.setColumn(i, 0xFF); // Set each column to 0xFF (all LEDs on)
    }
    mx.update(); // Update the display
  } else { // Switch is not pressed
    Serial.println("Switch Released - Clearing display");
    mx.clear(); // Clear the display
    mx.update(); // Update the display
  }
  delay(100); // Small delay to debounce switch
}