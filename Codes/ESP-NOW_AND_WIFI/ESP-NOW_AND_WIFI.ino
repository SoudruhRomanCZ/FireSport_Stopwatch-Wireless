#include "Arduino.h"
#include <WiFi.h>
#include <esp_now.h>
#include <LedControl.h>
#include <Firebase_ESP_Client.h>
#include <WiFiManager.h> // Include the WiFiManager library
#include "FirebaseConfig.h" // Include your Firebase configuration

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

// Timer variables
volatile bool timing[2] = {false, false}; // Timing states for both displays
volatile unsigned long elapsedTime[2] = {0, 0}; // Elapsed times for both timers
volatile unsigned long startTime[2] = {0, 0}; // Start times for both timers
bool started = false;

int hundreds[2] = {0, 0};
int seconds[2] = {0, 0};
int minutes[2] = {0, 0};

// Button pins
#define START_PIN 2
#define RESTART_PIN 4
// MAX7219 own SPI pins
#define DATA_IN 6
#define CLK 7
#define CS_PIN 8
LedControl lc = LedControl(DATA_IN, CLK, CS_PIN, 2);

// Structure to receive data
typedef struct struct_message {
    int id; // ID of the sender
    char message[20]; // Message sent from the sender
} struct_message;

struct_message incomingData;

// Callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
    memcpy(&incomingData, incomingData, sizeof(incomingData));
    Serial.printf("Received message: %s from ID: %d\n", incomingData.message, incomingData.id);
    
    // Stop timers based on the received ID
    if (incomingData.id == 1) {
        stopTimer(0);
    } else if (incomingData.id == 2) {
        stopTimer(1);
    }
}

void setup() {
  while (!Serial) {}
  Serial.begin(115200);

  pinMode(START_PIN, INPUT_PULLUP);
  pinMode(RESTART_PIN, INPUT_PULLUP);

  lc.shutdown(0, false);
  lc.shutdown(1, false);
  lc.setIntensity(0, 15);
  lc.setIntensity(1, 15);
  lc.clearDisplay(0);
  lc.clearDisplay(1);
  Serial.println("MAX7219 Initialized");

  // Initialize WiFiManager
  WiFiManager wifiManager;
  //first parameter is name of access point, second is the password
  // wifiManager.autoConnect("AP-NAME", "AP-PASSWORD"); // secured way
  wifiManager.autoConnect("SDH_Cvrcovice"); // not secured way
  wifiManager.autoConnect();

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
    // Initialize ESP-NOW
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_now_register_recv_cb(OnDataRecv); // Register the callback function

    // Initialization of the displays to L:0:00:00 and P:0:00:00
    resetTimer();
    Serial.println("Display initialized");
}

void loop() {
    if (digitalRead(START_PIN) == LOW && !timing[0] && !timing[1] && millis() - lastDebounceTimeStart > debounceDelay) {
        timing[0] = true; timing[1] = true; startTime[0] = millis(); startTime[1] = millis(); lastDebounceTimeStart = millis();
    } else if (digitalRead(START_PIN) == LOW && (timing[0] || timing[1]) && millis() - lastDebounceTimeStart > debounceDelay) {
        // Handle timestamp recording if needed
        lastDebounceTimeStart = millis();
    }
    if (digitalRead(RESTART_PIN) == LOW && millis() - lastDebounceTimeRestart > debounceDelay) {
        resetTimer(); lastDebounceTimeRestart = millis();
    }
}

void timingTask(void *parameter) {
    while (true) {
        for (int i = 0; i < 2; i++) {
            if (timing[i]) {
                elapsedTime[i] = millis() - startTime[i];
                // Update the display every 10 ms
                if (millis() - lastDisplayUpdate[i] >= displayUpdateInterval) {
                    // Calculate time components
                    hundreds[i] = (elapsedTime[i] / 10) % 100;
                    seconds[i] = (elapsedTime[i] / 1000) % 60;
                    minutes[i] = (elapsedTime[i] / 60000) % 60;
                    displayTime(i);
                    lastDisplayUpdate[i] = millis(); // Update the last display update time
                }
            }
        }
        vTaskDelay(1); // Yield to other tasks
    }
}

void stopTimer(int displayNumber) {
    timing[displayNumber] = false;
    Serial.println("Stopped Timer: " + String(displayNumber) + " : " + minutes[displayNumber] + " : " + seconds[displayNumber] + " : " + hundreds[displayNumber]);

    // Upload final time to Firebase
    String path = "timers/timer" + String(displayNumber + 1);
    Firebase.RTDB.setInt(&fbdo, path + "/elapsed", elapsedTime[displayNumber]);
    Firebase.RTDB.setInt(&fbdo, path + "/minutes", minutes[displayNumber]);
    Firebase.RTDB.setInt(&fbdo, path + "/seconds", seconds[displayNumber]);
    Firebase.RTDB.setInt(&fbdo, path + "/hundreds", hundreds[displayNumber]);
    Serial.println("Uploaded final time to Firebase");
}

void resetTimer() {
    for (int i = 0; i < 2; i++) {
        timing[i] = false; // Stop timers
        elapsedTime[i] = 0; // Reset elapsed time
        hundreds[i] = 0; // Reset hundreds
        seconds[i] = 0; // Reset seconds
        minutes[i] = 0; // Reset minutes
    }
    displayTime(0); // Display initial time for both displays
    displayTime(1);
    Serial.println("Both timers reset");
}

void displayTime(int displayNumber) {
    lc.setChar(displayNumber, 7, displayNumber == 0 ? 'L' : 'P', true);
    lc.setDigit(displayNumber, 6, minutes[displayNumber] % 10, true);
    lc.setDigit(displayNumber, 5, seconds[displayNumber] / 10, false);
    lc.setDigit(displayNumber, 4, seconds[displayNumber] % 10, true);
    lc.setDigit(displayNumber, 3, hundreds[displayNumber] / 10, false);
    lc.setDigit(displayNumber, 2, hundreds[displayNumber] % 10, false);
    lc.setChar(displayNumber, 1, ' ', false);
    lc.setChar(displayNumber, 0, ' ', false);
}

