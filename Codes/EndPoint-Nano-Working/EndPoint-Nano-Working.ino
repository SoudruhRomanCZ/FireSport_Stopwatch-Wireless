#include "printf.h"
#include <RF24.h>
#include <RF24Network.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

#define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW

// Pin Definitions for MAX7219
#define DIN_PIN  11
#define CS_PIN   10
#define SWITCH_PIN 2
#define SELECTOR_PIN 3
#define CE 9
#define CSN 8

// Create an instance for MAX7219
MD_MAX72XX mx = MD_MAX72XX(DIN_PIN, CS_PIN, 1);

// RF24 Network Setup
RF24 radio(CE, CSN);  // CE, CSN pins
RF24Network network(radio);
const uint16_t this_node[] ={01,02};   // Address of this node
const uint16_t other_node = 00;  // Address of the other node (base node)

void setup() {
  while (!Serial) {}// some boards need this because of native USB capability
  Serial.begin(115200); // Start serial communication
  Serial.println("Start of program");
  printf_begin();  // needed for RF24* libs' internal printf() calls

  pinMode(SWITCH_PIN, INPUT_PULLUP); // Set switch pin as input with pull-up
  pinMode(SELECTOR_PIN, INPUT_PULLUP);
  
  // Initialize MAX7219
  mx.begin(); // Initialize MAX7219
  mx.clear(); // Clear the display
  Serial.println("MAX7219 Initialized");

  // Initialize RF24 radio and network
  if (!radio.begin()) {
    Serial.println(F("Radio hardware not responding!"));
    while (1){displayError();} // Display "E" on the LED matrix when radio is not responding // Infinite loop to halt further execution
  }
  if (digitalRead(SELECTOR_PIN) == LOW){
    network.begin(90, this_node[0]);
    const char* message = "LEFT";
    Serial.println("Left RF24 node Initialized");
  } else if (digitalRead(SELECTOR_PIN) == HIGH) {
    network.begin(90, this_node[1]);
    const char* message = "RIGHT";
    Serial.println("Right RF24 node Initialized");
  } 
  radio.setAutoAck(true);
  radio.setRadiation(3, 1, 1);
  radio.printPrettyDetails();
  Serial.println("nRF24 Initialized");   
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

    // Send signal to the base node
    RF24NetworkHeader header(other_node, 32); // Message type 32
    bool ok = network.write(header, message, strlen(message) + 1); // Send message
    Serial.println(ok ? F("Signal sent to base node") : F("Failed to send signal"));

    // Wait for ACK
    network.update(); // Update network to handle incoming messages
    delay(100); // Small delay to allow for ACK response
  } else { // Switch is not pressed
    mx.clear(); // Clear the display
    mx.update(); // Update the display
  }
  
  delay(10); // Small delay to debounce switch
}

void displayError() {
  // Turn on all columns of the matrix
  for(int i = 0; i < 8; i++) {
    mx.setColumn(i, 0xFF); // Set each column to 0xFF (all LEDs on)
  }
  mx.update(); // Update the display
  delay(500); // Wait for 500 ms
  // Turn off all LEDs
  mx.clear(); // Clear the display
  mx.update(); // Update the display
  delay(500); // Wait for 500 ms
}