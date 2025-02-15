// Arduino nano base code to handle stopwatch
// #include "functions.h"
#include "Arduino.h"
#include "printf.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
#include <RF24.h>
#include <RF24Network.h>
#include <SoftwareSerial.h>
#include <DFPlayerMini_Fast.h>

#define START_PIN 2   // Start button pin
#define RESTART_PIN 3 // Restart button pin
#define MP3_TX 4      // MP3 player TX pin
#define MP3_RX 5      // MP3 player RX pin
#define SOUNDSTART 6  // Pin determining Soundstart
#define CSN 8         // Chip Select pin of nRF24 module (negate)
#define CE 9          // Chip Enable pin of nRF24 module
#define CS_PIN 10     // Chip select pin for MAX7219 display


// Define the number of devices in the chain
int numberOfHorizontalDisplays = 1; // Number of horizontal displays
int numberOfVerticalDisplays = 16;   // Number of vertical displays
int offset = numberOfVerticalDisplays*4; // number of displays / 2 = 1 row * 8 leds (pixels) == N*4
// User defined constants for durations in miliseconds
#define blinkDuration 500 // Duration to blink in milliseconds
#define minWaitTime 5000 // Minimum wait time when Automatic start by mp3 module
#define maxWaitTime 10000 // Maximum wait time when Automatic start by mp3 module
// User defined bool states to operate the program (for maximum universality)
bool soundStart = false; // Play determined sounds before start - when false = self start, when true - autostart

char szTime1[9];    // Buffer for elapsed time display for display 1
char szTime2[9];    // Buffer for elapsed time display for display 2
unsigned long startTime1 = 0; // Timer start time for display 1
unsigned long startTime2 = 0; // Timer start time for display 2
unsigned long elapsedTime1 = 0;
unsigned long elapsedTime2 = 0;
bool timing1 = false; // Timing state for display 1
bool timing2 = false; // Timing state for display 2
bool started = false; // Flag to determine when timing is started (for blinking purposes)
bool soundStarted = false; // Flag to indicate if the sound sequence has started

// MP3 variables
#define startByte 0x7E
#define endByte 0xEF
#define versionByte 0xFF
#define dataLength 0x06
#define infoReq 0x01
#define isDebug false

enum PlayState {
    IDLE,
    PLAY_READY,
    WAIT_READY,
    PLAY_SET,
    WAIT_SET,
    PLAY_START
};
PlayState currentState = IDLE;
int currentTrack = 0;
unsigned long startSoundTime = 0;

// MP3 player configuration
SoftwareSerial mp3(MP3_RX, MP3_TX); // RX, TX for MP3 player

// MAX7219 panel configuration
Max72xxPanel matrix = Max72xxPanel(CS_PIN, numberOfHorizontalDisplays, numberOfVerticalDisplays);

// RF24 configuration
RF24 radio(CE, CSN);  // CE, CSN pins
RF24Network network(radio);  // Network uses that radio
const uint16_t this_node = 00;   // Address of our node in Octal format 
const uint16_t other_node[] = {01, 02}; // Address of the other node

void setup() {
  matrix.setIntensity(15); // Set brightness
  matrix.setRotation(1); // Set rotation
  matrix.fillScreen(LOW); // Clear the display
  matrix.write(); // Update the display
  
  Serial.begin(115200);
  mp3.begin(9600); // Initialize MP3 player

  // Set the buttons as input with internal pull-up
  pinMode(START_PIN, INPUT_PULLUP);
  pinMode(RESTART_PIN, INPUT_PULLUP);
  pinMode(SOUNDSTART,INPUT_PULLUP);
  //if (SOUNDSTART) soundStart = false; else soundStart = true; // determine if it should start automaticly by sound via grounding pin D6 of arduino
  // nRF24 setting
  printf_begin();  // needed for RF24* libs' internal printf() calls
  while (!Serial) { /* some boards need this because of native USB capability */}
  radio.begin();
  if (!radio.begin()) { // Initialize RF24
    Serial.println(F("Radio hardware not responding!"));
    matrix.setCursor(0, 0); // Set cursor to the start for display 1
    matrix.print("Radio Fail"); // Print error on display  
    matrix.write(); // Update the display
    while (1) { /* hold in infinite loop */ }
  }
  network.begin(/*channel*/90, /*node address*/ this_node);
  radio.setAutoAck(true);
  radio.setPALevel(3, 1);
  radio.setDataRate(1);
  radio.printPrettyDetails();
  // Display initial time
  displayInitialTime();
}

void loop() {
  network.update(); // Update network to handle incoming messages
  handleReceivedMessage();

  if (digitalRead(RESTART_PIN) == LOW) { //restarts the program
    resetTimer();
  }

  if (digitalRead(START_PIN) == LOW && !timing1 && !timing2 && !soundStarted) { //starts the timer for display 1
    startTimer();
  }else if (soundStarted) { // Check if the sound sequence is currently running
    playSequence(); // Call the playSequence function
  }

  if (timing1) { // Update the display if timing for display 1
    countTime(1);
  }

  if (timing2) { // Update the display if timing for display 2
    countTime(2);
  }
  
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
  if (soundStart == true) {
    playSequence(); // Play predefined sounds if the flag is set
  } else {
    startTime1 = millis(); // Start the timer for display 1
    startTime2 = millis(); // Start the timer for display 2
    timing1 = true; // Set timing to true for display 1
    timing2 = true; // Set timing to true for display 2
    started = true;
    Serial.println("Both timers started");
  }
}

void stopTimer(int displayNumber) { // Function to stop the timer
  if (displayNumber == 1) {
    timing1 = false; // Set timing to false for display 1
  } else if (displayNumber == 2) {
    timing2 = false; // Set timing to false for display 2
  }
  Serial.println("Stopped Timer: " + displayNumber);
}

void resetTimer() { // Function to reset both timers and displays
  stopTimer(1); // Stop the timer for display 1
  stopTimer(2); // Stop the timer for display 2
  
  startTime1 = 0; // Reset start time for display 1
  startTime2 = 0; // Reset start time for display 2
  started = false;
  soundStarted = false;
  currentState = IDLE;

  displayInitialTime();
  Serial.println("Both timers reset");
}

void playSequence() {
  soundStarted = true;
    switch (currentState) {
        case IDLE:
            Serial.println("State: IDLE - Starting track 1");
            // Start playing the first track
            sendCommand(0x03, 0, 1); // Command to play track 1
            startSoundTime = millis(); // Record the start time
            currentState = WAIT_READY; // Move to waiting state for track 1
            break;

        case WAIT_READY:
            // Check if enough time has passed
            if (millis() - startSoundTime >= random(minWaitTime, maxWaitTime)) {
                Serial.println("Transitioning to PLAY_SET - Enough time passed for track 1");
                currentState = PLAY_SET; // Move to play the second track
            }
            break;

        case PLAY_SET:
            Serial.println("State: PLAY_SET - Starting track 2");
            sendCommand(0x03, 0, 2); // Command to play track 2
            startSoundTime = millis(); // Record the start time
            currentState = WAIT_SET; // Move to waiting state for track 2
            break;

        case WAIT_SET:
            // Check if enough time has passed
            if (millis() - startSoundTime >= random(minWaitTime, maxWaitTime)) {
                Serial.println("Transitioning to PLAY_START - Enough time passed for track 2");
                currentState = PLAY_START; // Move to play the third track
            }
            break;

        case PLAY_START:
            Serial.println("State: PLAY_START - Starting track 3");
            sendCommand(0x03, 0, 3); // Command to play track 3
            startTime1 = millis(); // Start the timer for display 1
            startTime2 = millis(); // Start the timer for display 2
            timing1 = true; // Set timing to true for display 1
            timing2 = true; // Set timing to true for display 2
            started = true;
            currentState = IDLE; // Reset to IDLE after playing
            Serial.println("Transitioning to IDLE - All tracks played");
            soundStarted = false;
            break;

        default:
            Serial.println("Unknown state!");
            break;
    }
}

void sendCommand(byte Command, byte Param1, byte Param2) {
  // Calculate the checksum
  unsigned int checkSum = -(versionByte + dataLength + Command + infoReq + Param1 + Param2);

  // Construct the command line
  byte commandBuffer[10] = { startByte, versionByte, dataLength, Command, infoReq, Param1, Param2,
      highByte(checkSum), lowByte(checkSum), endByte };

  for (int cnt = 0; cnt < 10; cnt++) {
    mp3.write(commandBuffer[cnt]);
  }

  // Delay needed between successive commands
  delay(30);
}

void countTime(int displayNumber) { // Function to count elapsed time, and then call displayTime() 
  if (displayNumber == 1) {
    elapsedTime1 = millis() - startTime1; // Calculate elapsed time for display 1
    int hundreds = (elapsedTime1 / 10) % 100; // Get hundredths
    int seconds = (elapsedTime1 / 1000) % 120; // Get seconds

    // Format the time as mm:ss
    sprintf(szTime1, "L:%03d:%02d", seconds, hundreds);

    // Send the timing data to ESP8266 via UART
    Serial.print("T1:");
    Serial.print(szTime1);
    Serial.print("\n"); // Newline to separate messages

  } else if (displayNumber == 2) {
    elapsedTime2 = millis() - startTime2; // Calculate elapsed time for display 2
    int hundreds = (elapsedTime2 / 10) % 100; // Get hundredths
    int seconds = (elapsedTime2 / 1000) % 120; // Get seconds

    // Format the time as mm:ss
    sprintf(szTime2, "P:%03d:%02d", seconds, hundreds);

    // Send the timing data to ESP8266 via UART
    Serial.print("T2:");
    Serial.print(szTime2);
    Serial.print("\n"); // Newline to separate messages
  }
  
  // Display the elapsed time
  displayTime();
}

void displayTime() { // Function to display both elapsed times
  matrix.fillScreen(LOW); // Clear the entire display

  // Display elapsed time for display 1
  matrix.setCursor(0, 0); // Set cursor to the start for display 1
  matrix.print(szTime1); // Print the elapsed time for display 1

  // Display elapsed time for display 2
  matrix.setCursor(48, 0); // Set cursor to the start for display 2
  matrix.print(szTime2); // Print the elapsed time for display 2

  matrix.write(); // Update the display
}

void blinkTime(int displayNumber) {
  static unsigned long lastBlinkTime = 0; // Store the last blink time
  static bool blinkState = false; // Track the blink state

  if (millis() - lastBlinkTime >= blinkDuration) {
    lastBlinkTime = millis(); // Update the last blink time
    blinkState = !blinkState; // Toggle blink state

    // Clear the display and redraw both times
    matrix.fillScreen(LOW); // Clear the entire display
    if (displayNumber == 1) {
      // Blink first time
      if (blinkState) {
        matrix.setCursor(0, 0); // Set cursor to the start for display 1
        matrix.print(szTime1); // Print the elapsed time for display 1
      }
      // Display second time
      matrix.setCursor(offset, 0); // Set cursor to the start for display 2
      matrix.print(szTime2); // Print the elapsed time for display 2
    } else if (displayNumber == 2) {
      // Display first time
      matrix.setCursor(0, 0); // Set cursor to the start for display 1
      matrix.print(szTime1); // Print the elapsed time for display 1
      // Blink second time
      if (blinkState) {
        matrix.setCursor(offset, 0); // Set cursor to the start for display 2
        matrix.print(szTime2); // Print the elapsed time for display 2
      }
    }
    matrix.write(); // Update the display
  }
}

void displayInitialTime(){
  matrix.fillScreen(LOW); // Clear the display
  matrix.setCursor(0, 0); // Set cursor to the start
  matrix.print("L:000:00"); // Print initial time for display 1
  matrix.setCursor(48, 0); // Set cursor to the start for display 2
  matrix.print("P:000:00"); // Print initial time for display 2
  matrix.write(); // Update the display
}

void handleReceivedMessage() {
  // Check for incoming messages
  RF24NetworkHeader header;
  if (network.available()) {
    network.peek(header); // Peek to get the header
    char dataBuffer[32]; // Buffer to store incoming data
    network.read(header, &dataBuffer, sizeof(dataBuffer)); // Read the incoming data

    // Stop the timer based on the sender node
    if (header.from_node == 1) {
      stopTimer(1); // Stop timer for display 1 if message is from node 1
    }
    // Stop the timer based on the sender node
    if (header.from_node == 2) {
      stopTimer(2); // Stop timer for display 2 if message is from node 2
    }
    // Start or restart timer, Node3 is starting pistol
    if (header.from_node == 3) {
      if (dataBuffer == "START"&& !timing1 && !timing2 && !soundStarted) soundStart = false; startTimer();
      if (dataBuffer == "RESET") resetTimer(); 
    }
    // The library automatically sends an ACK if the message was processed correctly
  }
}
/*

*/
void displayErrorMessage(const char* message) {
  int messageLength = strlen(message);
  int displayWidth = 8; // Width of the display in characters (8x8 matrix)
  
  // Scroll the message across the display
  for (int position = displayWidth; position < messageLength + displayWidth; position++) {
    matrix.fillScreen(LOW); // Clear the display
    for (int i = 0; i < displayWidth; i++) {
      if (position - i >= 0 && position - i < messageLength) {
        matrix.drawChar(displayWidth - 1 - i, 0, message[position - i - 1], HIGH, LOW, 1);
      }
    }
    matrix.write(); // Update the display
    delay(200); // Adjust speed of scrolling
  }
}