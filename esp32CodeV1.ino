#include <WiFi.h>
#include <HTTPClient.h>
#include "DHT.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// === Sensor Pins and Types ===
#define DHTPIN 4
#define DHTTYPE DHT11
#define SOIL_PIN 6

DHT dht(DHTPIN, DHTTYPE);

// === WiFi Setup ===
const char* ssid = "COMPUTERINATOR";
const char* password = "PerrydaPlatypus";
const char* serverURL = "http://192.168.0.102:8080/";

// === OLED Setup ===
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1

TwoWire oledWire = TwoWire(0);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &oledWire, OLED_RESET);

void setup() {
  Serial.begin(115200);

  // Initialize sensor
  dht.begin();
  pinMode(SOIL_PIN, INPUT);

  // Initialize OLED on custom pins
  oledWire.begin(9, 10);  // SDA = 9, SCL = 10
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED display failed"));
    while (true);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("Connecting WiFi...");
  display.startscrollleft(0x00, 0x03);
  display.display();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  display.stopscroll();
  Serial.println("\nConnected to WiFi!");

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Connected!");
  display.display();
  delay(1000);
}

unsigned long lastDisplayUpdate = 0;
const int displayInterval = 3000;  // 3 seconds

void animateDisplayUpdate(float temp, float humidity, float moisture) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Temp: " + String(temp, 1) + " C");
  display.println("Hum:  " + String(humidity, 1) + " %");
  display.println("Soil: " + String(moisture, 1) + " %");
  display.display();
}

void loop() {
  static unsigned long lastPost = 0;
  static const int postInterval = 5000;

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int rawSoil = analogRead(SOIL_PIN);
  float moisture = map(rawSoil, 4095, 0, 0, 100);

  display.setTextSize(1.5);

  static unsigned long lastDisplayUpdate = 0;
  if (millis() - lastDisplayUpdate > 2000) {
    lastDisplayUpdate = millis();
    animateDisplayUpdate(temperature, humidity, moisture);
  }

  // === Server Update Every 5 Seconds ===
  if (millis() - lastPost >= postInterval) {
    lastPost = millis();
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverURL);
      http.addHeader("Content-Type", "application/json");

      String json = "{\"temperature\":";
      json += temperature;
      json += ",\"humidity\":";
      json += humidity;
      json += ",\"soil\":";
      json += moisture;
      json += "}";

      int code = http.POST(json);
      Serial.printf("POST code: %d\n", code);
      http.end();
    } else {
      Serial.println("WiFi not connected");
    }
  }
}