#include <WiFiS3.h>

const char* ssid = "Project";
const char* password = "12345678";

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Attempt to connect to WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);

  // Wait until the connection is established
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  // Print the IP address
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Your main code goes here
}
