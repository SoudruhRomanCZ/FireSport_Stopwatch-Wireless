#include "Arduino.h"
#include "printf.h"
#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
#include <SoftwareSerial.h>
#include <LedControl.h> // Include the LedControl library for MAX7219

#define START_PIN 2   // Start button pin
#define RESTART_PIN 3 // Restart button pin
#define CSN 9         // Chip Select pin of nRF24 module (negate)
#define CE 10          // Chip Enable pin of nRF24 module (nRF24L01)

// MAX7219 configuration
#define DATA_IN 6    // Data pin for MAX7219
#define CLK 7        // Clock pin for MAX7219
#define CS_PIN 8     // Chip Select pin for MAX7219
LedControl lc = LedControl(DATA_IN, CLK, CS_PIN, 2); // 2 devices connected

#define blinkDuration 1000 // Duration to blink in milliseconds

unsigned long startTime1 = 0; // Timer start time for display 1
unsigned long startTime2 = 0; // Timer start time for display 2
unsigned long elapsedTime1 = 0;
unsigned long elapsedTime2 = 0;
bool timing1 = false; // Timing state for display 1
bool timing2 = false; // Timing state for display 2
bool started = false; // Flag to determine when timing is started

int hundreds1 = 0;
int seconds1 = 0;
int minutes1 = 0;

int hundreds2 = 0;
int seconds2 = 0;
int minutes2 = 0;

// Debounce variables
unsigned long lastDebounceTimeRestart = 0; // Last time the restart button was pressed
const unsigned long debounceDelay = 500; // Debounce time in milliseconds

// RF24 configuration
RF24 radio(CE, CSN);  // CE, CSN pins
RF24Network network(radio);  // Network uses that radio
const uint16_t this_node = 00;   // Address of our node in Octal format 
const uint16_t other_node[] = {01, 02, 03}; // Address of the other node

void setup() {
  while (!Serial) {}// some boards need this because of native USB capability
  Serial.begin(115200); // Start serial communication
  Serial.println("Start of program");
  printf_begin();  // needed for RF24* libs' internal printf() calls

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
  Serial.println("MAX7219 Initialized");
  delay(1000);
  // nRF24 setting
  if (!radio.begin()) { // Initialize RF24
    Serial.println(F("Radio hardware not responding!"));
    displayErrorMessage();
    while (1) { /* hold in infinite loop */ }
  }
  network.begin(/*channel*/90, /*node address*/ this_node);
  radio.setAutoAck(true);
  radio.setRadiation(3, 1, 1);
  radio.printPrettyDetails(); 
  Serial.println("nRF24 Initialized");

  // Display initial time
  displayInitialTime();
}

void loop() {
  network.update(); // Update network to handle incoming messages
  handleReceivedMessage();
  if (digitalRead(START_PIN) == LOW && !timing1 && !timing2){ startTimer();} // starting timer via button on D2
  // Read the restart button with debounce
  if (digitalRead(RESTART_PIN) == LOW && millis() - lastDebounceTimeRestart > debounceDelay) {
    lastDebounceTimeRestart = millis(); // Update the last debounce time
    resetTimer();
  }
  if (timing1) { countTime(1); } // Update the display if timing for display 1
  if (timing2) { countTime(2); } // Update the display if timing for display 2
  
  // Check if both timers are stopped
  if (!timing1 && !timing2 && started) {
    if (elapsedTime1 > elapsedTime2) {
      // Left timer is faster
      blinkTime(1); // Blink right time
    } else {  
      // Right timer is faster or equal
      blinkTime(2); // Blink left time
    }
  } 
}

void startTimer() { // Function to start both timers
  startTime1 = millis(); // Start the timer for display 1
  startTime2 = millis(); // Start the timer for display 2
  timing1 = true; // Set timing to true for display 1
  timing2 = true; // Set timing to true for display 2
  started = true;
  Serial.println("Both timers started");
}

void stopTimer(int displayNumber) { // Function to stop the timer
  if (displayNumber == 1) {
    timing1 = false; // Set timing to false for display 1
  } else if (displayNumber == 2) {
    timing2 = false; // Set timing to false for display 2
  }
  Serial.println("Stopped Timer: " + String(displayNumber));
}

void resetTimer() { // Function to reset both timers and displays
  stopTimer(1); // Stop the timer for display 1
  stopTimer(2); // Stop the timer for display 2
  displayInitialTime();
  startTime1 = 0; // Reset start time for display 1
  startTime2 = 0; // Reset start time for display 2
  started = false;
  Serial.println("Both timers reset");
}

void countTime(int displayNumber) { // Function to count elapsed time
  if (displayNumber == 1) {
    elapsedTime1 = millis() - startTime1; // Calculate elapsed time for display 1
    hundreds1 = (elapsedTime1 / 10) % 100; // Get hundredths
    seconds1 = (elapsedTime1 / 1000) % 60; // Get seconds
    minutes1 = (elapsedTime1 / 60000) % 9; // Get minutes
    Serial.print("Displaying time1 L: ");
    Serial.print(minutes1);
    Serial.print(":");
    Serial.print(seconds1);
    Serial.print(",");
    Serial.println(hundreds1);
    // Display time in mm:ss format on the first display
    lc.setChar(0, 7, 'L', true); // Units of minutes
    lc.setDigit(0, 6, minutes1 % 10, true); // Units of minutes
    lc.setDigit(0, 5, seconds1 / 10, false); // Tens of seconds
    lc.setDigit(0, 4, seconds1 % 10, true); // Units of seconds
    lc.setDigit(0, 3, hundreds1 / 10, false); // Tens of milliseconds
    lc.setDigit(0, 2, hundreds1 % 10, false); // Units of milliseconds
    lc.setChar(0, 1, ' ', false);
    lc.setChar(0, 0, ' ', false);

  } else if (displayNumber == 2) {
    elapsedTime2 = millis() - startTime2; // Calculate elapsed time for display 2
    hundreds2 = (elapsedTime2 / 10) % 100; // Get hundredths
    seconds2 = (elapsedTime2 / 1000) % 60; // Get seconds
    minutes2 = (elapsedTime2 / 60000) % 60; // Get minutes
    Serial.print("Displaying time2 P: ");
    Serial.print(minutes2);
    Serial.print(":");
    Serial.print(seconds2);
    Serial.print(",");
    Serial.println(hundreds2);
    // Display time in mm:ss format on the second display
    lc.setChar(1, 7, 'P', true); // Units of minutes
    lc.setDigit(1, 6, minutes2 % 10, true); // Units of minutes
    lc.setDigit(1, 5, seconds2 / 10, false); // Tens of seconds
    lc.setDigit(1, 4, seconds2 % 10, true); // Units of seconds
    lc.setDigit(1, 3, hundreds2 / 10, false); // Tens of milliseconds
    lc.setDigit(1, 2, hundreds2 % 10, false); // Units of milliseconds
    lc.setChar(1, 1, ' ', false);
    lc.setChar(1, 0, ' ', false);
  }
}

void blinkTime(int displayNumber) {
  static unsigned long lastBlinkTime = 0; // Store the last blink time
  static bool blinkState = false; // Track the blink state

  if (millis() - lastBlinkTime >= blinkDuration) {
    lastBlinkTime = millis(); // Update the last blink time
    blinkState = !blinkState; // Toggle blink state
    
    if (displayNumber == 1) {
      if (blinkState) {
        Serial.print("Displaying time1 L: ");
        Serial.print(minutes1);
        Serial.print(":");
        Serial.print(seconds1);
        Serial.print(",");
        Serial.println(hundreds1);
        // Display time in mm:ss format on the first display
        lc.setChar(0, 7, 'L', true); // Units of minutes
        lc.setDigit(0, 6, minutes1 % 10, true); // Units of minutes
        lc.setDigit(0, 5, seconds1 / 10, false); // Tens of seconds
        lc.setDigit(0, 4, seconds1 % 10, true); // Units of seconds
        lc.setDigit(0, 3, hundreds1 / 10, false); // Tens of milliseconds
        lc.setDigit(0, 2, hundreds1 % 10, false); // Units of milliseconds
        lc.setChar(0, 1, ' ', false);
        lc.setChar(0, 0, ' ', false);; // Display the first timer
      } else {
        lc.clearDisplay(0); // Clear the first display
      }
    }

    // Clear the display and redraw both times
    if (displayNumber == 2) {
      if (blinkState) {
        Serial.print("Displaying time2 P: ");
        Serial.print(minutes2);
        Serial.print(":");
        Serial.print(seconds2);
        Serial.print(",");
        Serial.println(hundreds2);
        // Display time in mm:ss format on the second display
        lc.setChar(1, 7, 'P', true); // Units of minutes
        lc.setDigit(1, 6, minutes2 % 10, true); // Units of minutes
        lc.setDigit(1, 5, seconds2 / 10, false); // Tens of seconds
        lc.setDigit(1, 4, seconds2 % 10, true); // Units of seconds
        lc.setDigit(1, 3, hundreds2 / 10, false); // Tens of milliseconds
        lc.setDigit(1, 2, hundreds2 % 10, false); // Units of milliseconds
        lc.setChar(1, 1, ' ', false);
        lc.setChar(1, 0, ' ', false);
      } else {
        lc.clearDisplay(1); // Clear the second display
      }
    }
  }
}

void displayInitialTime() {
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
    Serial.println("Display initilized");
}

void handleReceivedMessage() {
  // Check for incoming messages
  RF24NetworkHeader header;
  if (network.available()) {
    network.peek(header); // Peek to get the header
    char dataBuffer[32]; // Buffer to store incoming data
    network.read(header, &dataBuffer, sizeof(dataBuffer)); // Read the incoming data

    // Stop the timer based on the sender node
    if (header.from_node == 1){ stopTimer(1);} // Stop timer for display 1 if message is from node 1
    if (header.from_node == 2){ stopTimer(2);} // Stop timer for display 2 if message is from node 2
    // Start or restart timer, Node3 is starting pistol
    if (header.from_node == 3) {
      if (dataBuffer == "START" && !timing1 && !timing2) { startTimer();} 
      if (dataBuffer == "RESET") { resetTimer();} 
    }
    // The library automatically sends an ACK if the message was processed correctly
  }
}

void displayErrorMessage() {
    // Test with simple characters
    lc.setChar(0, 7, 'E', false);
    lc.setChar(0, 6, 'A', false);
    lc.setChar(0, 5, 'A', false);
    lc.setChar(0, 4, '0', false);
    lc.setChar(0, 3, 'A', false);
    lc.setChar(0, 2, ' ', false);
    lc.setChar(0, 1, ' ', false);
    lc.setChar(0, 0, ' ', false);

    lc.setChar(1, 7, ' ', false);
    lc.setChar(1, 6, ' ', false);
    lc.setChar(1, 5, ' ', false);
    lc.setChar(1, 4, 'P', false);
    lc.setChar(1, 3, 'A', false);
    lc.setChar(1, 2, 'D', false);
    lc.setChar(1, 1, '1', false);
    lc.setChar(1, 0, '0', false);
}