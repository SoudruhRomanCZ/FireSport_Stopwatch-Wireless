#include <Wire.h>
#include <ESP8266WiFi.h>
#include "Adafruit_OV7670.h"

// WiFi credentials
const char* ssid = "SDH Cvrčovice"; // Replace with your network SSID
const char* password = "Cvrcek"; // Replace with your network password

WiFiServer server(80); // Create a server that listens on port 80

String timingData1 = "00:00"; // Buffer for timing data from Arduino (display 1)
String timingData2 = "00:00"; // Buffer for timing data from Arduino (display 2)

// CAMERA CONFIG -----------------------------------------------------------
OV7670_arch arch = {.timer = TCC1, .xclk_pdec = false};
OV7670_pins pins = {.enable = PIN_PCC_D8, .reset = PIN_PCC_D9,
                    .xclk = PIN_PCC_XCLK};
#define CAM_I2C Wire1 // Second I2C bus next to PCC pins

#define CAM_SIZE OV7670_SIZE_DIV2 // QVGA (320x240 pixels)
#define CAM_MODE OV7670_COLOR_RGB // RGB plz

Adafruit_OV7670 cam(OV7670_ADDR, &pins, &CAM_I2C, &arch);

// SETUP - RUNS ONCE ON STARTUP --------------------------------------------
void setup() {
  Serial.begin(115200); // Start the Serial communication
  Serial.setTimeout(100); // Set timeout for Serial reads

  // Set up the ESP8266 in access point mode
  WiFi.softAP(ssid, password);
  Serial.println("Access Point started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP()); // Print the IP address of the access point

  server.begin(); // Start the server
  Serial.println("Server started");

  // Initialize camera
  OV7670_status status = cam.begin(CAM_MODE, CAM_SIZE, 30.0);
  if (status != OV7670_STATUS_OK) {
    Serial.println("Camera begin() fail");
    for(;;);
  }
}

// MAIN LOOP - RUNS REPEATEDLY UNTIL RESET OR POWER OFF --------------------
void loop() {
  // Check for incoming serial data
  if (Serial.available()) {
    String incomingData = Serial.readStringUntil('\n'); // Read until newline
    Serial.println("Received from Serial: " + incomingData); // Print received data for debugging

    // Process the incoming data
    if (incomingData.startsWith("T1:")) {
      timingData1 = incomingData.substring(3); // Get the timing data for display 1
    } else if (incomingData.startsWith("T2:")) {
      timingData2 = incomingData.substring(3); // Get the timing data for display 2
    }
  }

  // Check for incoming clients
  WiFiClient client = server.available(); 
  if (client) {
    Serial.println("New Client.");
    String currentLine = ""; // Make a String to hold incoming data from the client

    while (client.connected()) {
      if (client.available()) {
        char c = client.read(); // Read a byte from the client
        Serial.write(c); // Echo back to the serial monitor

        // If you get a newline character, then the client has sent the end of the request
        if (c == '\n') {
          // Send the HTTP response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();

          // Send the HTML page with JavaScript for AJAX updates
          client.println("<!DOCTYPE html>");
          client.println("<html>");
          client.println("<head><title>Timing Data</title>");
          client.println("<script>");
          client.println("function updateData() {");
          client.println("  fetch('/timing').then(response => response.json()).then(data => {");
          client.println("    document.getElementById('timing1').innerText = data.timing1;");
          client.println("    document.getElementById('timing2').innerText = data.timing2;");
          client.println("  });");
          client.println("  document.getElementById('camera').src = '/stream?' + new Date().getTime ();");
          client.println("}");
          client.println("setInterval(updateData, 1000);"); // Update every 1 second
          client.println("</script>");
          client.println("</head>");
          client.println("<body><h1>Timing Data</h1>");
          client.println("<p id='timing1'>Display 1 Time: " + timingData1 + "</p>");
          client.println("<p id='timing2'>Display 2 Time: " + timingData2 + "</p>");
          client.println("<h2>Camera Feed</h2>");
          client.println("<img id='camera' src='/stream' />"); // Add camera feed
          client.println("</body></html>");

          // Break the while loop after handling the request
          break; 
        }
      }
    }

    // Close the client connection after serving the request
    client.stop();
    Serial.println("Client disconnected.");
  }

  // Handle timing data request
  client = server.available();
  if (client && client.uri() == "/timing") {
    client.sendHeader("Access-Control-Allow-Origin", "*");
    client.send(200, "application/json");
    client.println("{\"timing1\":\"" + timingData1 + "\",\"timing2\":\"" + timingData2 + "\"}");
  }

  // Handle camera stream request
  client = server.available();
  if (client && client.uri() == "/stream") {
    handleStream(client);
  }
}

// Handle streaming
void handleStream(WiFiClient client) {
  client.sendHeader("Access-Control-Allow-Origin", "*");
  client.send(200, "multipart/x-mixed-replace; boundary=frame");
  
  while (true) {
    cam.capture(); // Capture frame from camera

    // Convert the frame to JPEG format
    uint8_t* buffer = cam.getBuffer();
    size_t length = cam.width() * cam.height() * 2; // Assuming RGB565 format

    // Send the JPEG header
    client.sendContent("--frame\r\n");
    client.sendContent("Content-Type: image/jpeg\r\n");
    client.sendContent("Content-Length: " + String(length) + "\r\n\r\n");

    // Send the image data
    client.sendContent((const char*)buffer, length);

    client.sendContent("\r\n");
    delay(30); // Adjust frame rate as needed
  }
}