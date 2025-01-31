#include <SPI.h>
#include <LedControl.h> // Include the LedControl library for MAX7219

#define START_PIN 2
#define RESTART_PIN 3
// MAX7219 configuration
#define DATA_IN 11    // Data pin for MAX7219
#define CLK 13        // Clock pin for MAX7219
#define CS_PIN 10     // Chip Select pin for MAX7219
LedControl lc = LedControl(DATA_IN, CLK, CS_PIN, 2); // 2 devices connected

unsigned long startTime = 0; // Timer start time
bool timing = false; // Timing state

// Debounce variables
unsigned long lastDebounceTimeStart = 0; // Last time the start button was pressed
unsigned long lastDebounceTimeRestart = 0; // Last time the restart button was pressed
const unsigned long debounceDelay = 500; // Debounce time in milliseconds

void setup() {
  // Set the buttons as input with internal pull-up
  pinMode(START_PIN, INPUT_PULLUP);
  pinMode(RESTART_PIN, INPUT_PULLUP);

  // Initialize the MAX7219
  lc.shutdown(0, false);       // Wake up the first MAX7219
  lc.shutdown(1, false);       // Wake up the second MAX7219
  lc.setIntensity(0, 15);      // Set brightness level for the first display (0 is min, 15 is max)
  lc.setIntensity(1, 15);      // Set brightness level for the second display
  lc.clearDisplay(0);          // Clear display register for the first display
  lc.clearDisplay(1);          // Clear display register for the second display

  Serial.begin(115200);
  Serial.println("MAX7219 7-Segment Display Test Started");
  displayInitialTime(); // Optionally display initial time
}

void loop() {
    // Read the start button with debounce
    if (digitalRead(START_PIN) == LOW) {
        if (millis() - lastDebounceTimeStart > debounceDelay) {
            lastDebounceTimeStart = millis(); // Update the last debounce time
            if (!timing) {
                startTimer();
            } else {
                // If timing is true, you can stop the timer or do something else
                timing = false; // Stop the timer
                Serial.println("Timer stopped");
            }
        }
    }

    // Read the restart button with debounce
    if (digitalRead(RESTART_PIN) == LOW) {
        if (millis() - lastDebounceTimeRestart > debounceDelay) {
            lastDebounceTimeRestart = millis(); // Update the last debounce time
            resetTimer();
        }
    }

    if (timing) {
        countTime(); // Update the display with elapsed time
    }
}

void startTimer() { // Function to start the timer
  startTime = millis(); // Start the timer
  timing = true; // Set timing to true
  Serial.println("Timer started");
}

void countTime() { // Function to count elapsed time
  unsigned long elapsedTime = millis() - startTime; // Calculate elapsed time
  int hundreds = (elapsedTime / 10) % 100; // Get tens of milliseconds
  int seconds = (elapsedTime / 1000) % 60; // Get seconds
  int minutes = (elapsedTime / 60000) % 60; // Get minutes

  // Display time in mm:ss:hh format on the first display
  lc.setChar(0, 7, 'L', true); // Units of minutes
  lc.setDigit(0, 6, minutes % 10, true); // Units of minutes
  lc.setDigit(0, 5, seconds / 10, false); // Tens of seconds
  lc.setDigit(0, 4, seconds % 10, true); // Units of seconds
  lc.setDigit(0, 3, hundreds / 10, false); // Tens of milliseconds
  lc.setDigit(0, 2, hundreds % 10, false); // Units of milliseconds
  lc.setChar(0, 1, ' ', false);
  lc.setChar(0, 0, ' ', false);

  // Optionally, display the same time on the second display
  lc.setChar(1, 7, 'P', true); // Units of minutes
  lc.setDigit(1, 6, minutes % 10, true); // Units of minutes
  lc.setDigit(1, 5, seconds / 10, false); // Tens of seconds
  lc.setDigit(1, 4, seconds % 10, true); // Units of seconds
  lc.setDigit(1, 3, hundreds / 10, false); // Tens of milliseconds
  lc.setDigit(1, 2, hundreds % 10, false); // Units of milliseconds
  lc.setChar(1, 1, ' ', false);
  lc.setChar(1, 0, ' ', false);
}

void resetTimer() { // Function to reset the timer and displays
    timing = false; // Stop the timer
    lc.clearDisplay(0); // Clear the first display
    lc.clearDisplay(1); // Clear the second display
    startTime = 0; // Reset start time
    Serial.println("Timer reset");
    displayInitialTime(); // Optionally display initial time
}

void displayInitialTime() {

    // Optionally display initial time or a message on the displays
    lc.setChar(0, 0, ' ', false);
    lc.setChar(0, 1, ' ', false);
    lc.setChar(0, 2, '0', false);
    lc.setChar(0, 3, '0', false);
    lc.setChar(0, 4, '0', true);
    lc.setChar(0, 5, '0', false);
    lc.setChar(0, 6, '0', true);
    lc.setChar(0, 7, 'L', true);
    
    lc.setChar(1, 0, ' ', false);
    lc.setChar(1, 1, ' ', false);
    lc.setChar(1, 2, '0', false);
    lc.setChar(1, 3, '0', false);
    lc.setChar(1, 4, '0', true);
    lc.setChar(1, 5, '0', false);
    lc.setChar(1, 6, '0', true);
    lc.setChar(1, 7, 'P', true);
}