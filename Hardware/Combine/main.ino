// Arduino R4 WiFi code final 
// created by Amit Kumar Singh
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
#include <Servo.h>

#define DHT11PIN 6
#define DHTTYPE DHT11
#define LIGHT_SENSOR_PIN A1
#define SERVO_PIN 2
#define PIR_SENSOR_PIN 4
#define RELAY_PIN 3
#define RAIN_SENSOR_PIN A2

const char* ssid = "Project";
const char* password = "12345678";
const char* thingspeakServer = "api.thingspeak.com";
const String apiKey = "OEL5UOKAN250I5UT";

DHT dht(DHT11PIN, DHTTYPE);
Servo myServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiClient wifiClient;
HttpClient httpClient(wifiClient, thingspeakServer, 80);

WiFiServer server(80);

int buttonPin = 2; 
int buttonState = 0; 
int displayMode = 1;

bool automatedMode = true; // Automated mode by default
bool lightStatus = false; // Light off by default
int servoPosition = 0; // Servo at 0 degrees by default

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP); 
  pinMode(PIR_SENSOR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  myServo.attach(SERVO_PIN);
  digitalWrite(RELAY_PIN, LOW); // Ensure relay is initially off
  
  Serial.println("Initializing...");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");

  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    if (request.indexOf("/toggle_automated") != -1) {
      automatedMode = !automatedMode;
      client.print("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n");
      client.print(automatedMode ? "Manual Mode" : "Automated Mode");
    } else if (request.indexOf("/toggle_light") != -1) {
      lightStatus = !lightStatus;
      digitalWrite(RELAY_PIN, lightStatus ? HIGH : LOW);
      client.print("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n");
      client.print(lightStatus ? "Light OFF" : "Light ON");
    } else if (request.indexOf("/toggle_dim") != -1) {
      servoPosition = (servoPosition == 0) ? 180 : 0;
      myServo.write(servoPosition);
      client.print("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n");
      client.print(servoPosition == 0 ? "Bright" : "Dim");
    } else if (request.indexOf("/get_data") != -1) {
      String jsonData = getData();
      client.print("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
      client.print(jsonData);
    }

    client.stop();
  }

  buttonState = digitalRead(buttonPin); 

  if (buttonState == LOW) { 
    changeDisplayMode();    
    delay(500);             
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (automatedMode) {
      // Read Weather Data
      float temperatureDHT = dht.readTemperature();
      float humidity = dht.readHumidity();
      int rainSensorValue = analogRead(RAIN_SENSOR_PIN);
      int analogValue = analogRead(LIGHT_SENSOR_PIN);
      // Read PIR Data
      int pirValue = digitalRead(PIR_SENSOR_PIN);

      // Display Weather Data on Serial Monitor
      Serial.println("Weather Data:");
      Serial.print("Temperature (DHT11): ");
      Serial.print(temperatureDHT);
      Serial.println(" *C");
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.println("%");
      Serial.print("Rain Sensor: ");
      Serial.println(rainSensorValue);
      Serial.print("Analog Value (LDR): ");
      Serial.println(analogValue);
      Serial.print("PIR Value = ");
      Serial.print(pirValue);
      
      // Display Data on LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" LDR: ");
      lcd.print(analogValue);
      lcd.setCursor(0, 1);
      lcd.print("Rain: ");
      lcd.print(rainSensorValue);
      lcd.print(" PIR: ");
      lcd.print(pirValue);

      // Send Weather Data to ThingSpeak
      sendDataToThingSpeak(temperatureDHT, humidity, rainSensorValue, analogValue);




      if (analogValue > 545) {
        Serial.println(" => Bright");
        digitalWrite(RELAY_PIN, HIGH); // Turn relay off
      } else {
        Serial.println(" => Dark");
        digitalWrite(RELAY_PIN, LOW); // Turn relay on

        if (pirValue == HIGH) {
          myServo.write(180); // Move servo to 180 degrees
        } else {
          myServo.write(0); // Move servo to 0 degrees
        }
      }

      delay(500); // Delay between data readings
    }
  }
}

void sendDataToThingSpeak(float temperatureDHT, float humidity, int rainSensorValue, int analogValue) {
  if (WiFi.status() == WL_CONNECTED) {
    String url = "/update?api_key=" + apiKey;
    url += "&field1=" + String(temperatureDHT);
    url += "&field2=" + String(humidity);
    url += "&field3=" + String(rainSensorValue);
    url += "&field4=" + String(analogValue);

    Serial.println("Sending data to ThingSpeak...");
    Serial.println(url);

    httpClient.get(url.c_str());

    int statusCode = httpClient.responseStatusCode();
    String response = httpClient.responseBody();

    Serial.print("Status code: ");
    Serial.println(statusCode);
    Serial.print("Response: ");
    Serial.println(response);

    if (statusCode == 200) {
      Serial.println("Data sent to ThingSpeak successfully!");
    } else {
      Serial.println("Failed to send data to ThingSpeak.");
    }
  } else {
    Serial.println("WiFi Disconnected! Unable to send data to ThingSpeak.");
  }
}

void changeDisplayMode() {
  displayMode++; 

  if (displayMode > 5) { 
    displayMode = 1;
  }
}

String getData() {
  float temperatureDHT = dht.readTemperature();
  float humidity = dht.readHumidity();
  int rainSensorValue = analogRead(RAIN_SENSOR_PIN);
  int analogValue = analogRead(LIGHT_SENSOR_PIN);
  int pirValue = digitalRead(PIR_SENSOR_PIN);

  String jsonData = "{";
  jsonData += "\"temperatureDHT\":" + String(temperatureDHT) + ",";
  jsonData += "\"humidity\":" + String(humidity) + ",";
  jsonData += "\"rainSensor\":" + String(rainSensorValue) + ",";
  jsonData += "\"lightSensor\":" + String(analogValue) + ",";
  jsonData += "\"pirSensor\":" + String(pirValue) + ",";
  jsonData += "\"relayStatus\":" + String(digitalRead(RELAY_PIN)) + ",";
  jsonData += "\"servoPosition\":" + String(servoPosition) + ",";
  jsonData += "\"lightStatus\":" + String(lightStatus);
  jsonData += "}";

  return jsonData;
}
