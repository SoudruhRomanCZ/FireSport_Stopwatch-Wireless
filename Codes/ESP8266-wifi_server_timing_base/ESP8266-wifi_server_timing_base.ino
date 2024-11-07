#include <ESP8266WiFi.h>

const char* ssid = "SDH Cvrčovice"; // Replace with your network SSID
const char* password = "Cvrcek"; // Replace with your network password

WiFiServer server(80); // Create a server that listens on port 80

String timingData1 = "00:00"; // Buffer for timing data from Arduino (display 1)
String timingData2 = "00:00"; // Buffer for timing data from Arduino (display 2)
String node = ""; // Initialize node as an empty string
unsigned long changeCounter = 0; // Counter for node changes
unsigned long lastUpdateMillis = 0; // Store the last update time

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
}

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
    } else if (incomingData.startsWith("Node:")) {
      String newNode = incomingData.substring(5); 
      if (newNode != node) { // Check if the node value has changed
        node = newNode; // Update node value
        changeCounter++; // Increment change counter
        lastUpdateMillis = millis(); // Update the last change time
      }
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

          // Send the HTML page with auto-refresh
          client.println("<!DOCTYPE html>");
          client.println("<html>");
          client.println("<head><title>Timing Data</title>");
          client.println("<meta http-equiv=\"refresh\" content=\"1\">"); // Refresh every second
          client.println("</head>");
          client.println("<body><h1>Timing Data</h1>");
          client.println("<p>Display 1 Time: " + timingData1 + "</p>");
          client.println("<p>Display 2 Time: " + timingData2 + "</p>");
          client.println("<p>Node: " + node + "</p>"); // Display the current node value
          client.println("<p>Node Change Count: " + String(changeCounter) + "</p>"); // Display change count
          client.println("<p>Milliseconds since last node change: " + String(millis() - lastUpdateMillis) + "</p>"); // Display time since last change
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
}