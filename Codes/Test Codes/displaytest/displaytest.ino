#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>

#define CS_PIN 10  // Chip select pin

// Define the number of devices in the chain
int numberOfHorizontalDisplays = 1; // Number of horizontal displays
int numberOfVerticalDisplays = 4;   // Number of vertical displays
Max72xxPanel matrix = Max72xxPanel(CS_PIN, numberOfHorizontalDisplays, numberOfVerticalDisplays);

#define START_PIN 2

char szTime[9];    // Buffer for elapsed time display
unsigned long startTime = 0; // Timer start time
bool timing = false; // Timing state
bool started = false; // Timer started state

void setup() {
  matrix.setIntensity(15); // Set brightness
  matrix.setRotation(3); // Set rotation
  Serial.begin(115200);
  Serial.println("\n[Max72xxPanel Message Display]\nDisplaying elapsed time.");

  pinMode(START_PIN, INPUT_PULLUP);
  
  // Display initial time
  displayInitialTime();
}

void loop() {
  // Check for button presses
  if (digitalRead(START_PIN) == LOW) {
    if (!timing && !started) {
      startTimer();
    } else {
      timing = !timing; // Toggle timing
    }
  }

  // Update the display if timing
  if (timing) {
    displayTime();
  }
}

// Function to start the timer
void startTimer() {
  startTime = millis(); // Start the timer
  timing = true; // Set timing to true
  started = true; // Mark that the timer has started
  Serial.println("Timer started");
}

// Function to display elapsed time
void displayTime() {
  unsigned long elapsedTime = millis() - startTime; // Calculate elapsed time
  int hundreds = (elapsedTime / 10) % 100; // Get hundredths
  int seconds = (elapsedTime / 1000) % 120; // Get seconds

  // Format the time as mm:ss
  sprintf(szTime, "%02d,%02d", seconds, hundreds);
  Serial.print("Time sent to display: ");
  Serial.println(szTime);
  
  // Display the elapsed time
  displayElapsedTime();
}

// Function to blink the elapsed time
void blinkTime() {
  for (int i = 0; i < 5; i++) { // Blink 5 times
    matrix.fillScreen(LOW); // Clear the display
    matrix.setCursor(0, 0); // Set cursor to the start
    matrix.print(szTime); // Print the elapsed time
    matrix.write(); // Update the display
    delay(500); // Wait for half a second
    matrix.fillScreen(LOW); // Clear the display
    delay(500); // Wait for half a second
  }
}

// Function to display initial time
void displayInitialTime() {
  matrix.fillScreen(LOW); // Clear the display
  matrix.setCursor(0, 0); // Set cursor to the start
  matrix.print("00:00"); // Print initial time
  matrix.write(); // Update the display
}

// Function to display elapsed time
void displayElapsedTime() {
  matrix.fillScreen(LOW); // Clear the display
  matrix.setCursor(0, 0); // Set cursor to the start
  matrix.print(szTime); // Print the elapsed time
  matrix.write(); // Update the display
}