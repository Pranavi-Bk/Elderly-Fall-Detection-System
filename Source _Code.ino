#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <base64.h>
#include <math.h>

// ================== WiFi Credentials ==================
const char* ssid     = "******";       // 🔹 Your Wi-Fi SSID
const char* password = "**********";       // 🔹 Your Wi-Fi password

// ================== Twilio Account ==================
const String accountSID = "**********";
const String authToken  = "**********";
const String fromNumber = "**********";
const String toNumber   = "**********";

// ================== MPU6050 ==================
const uint8_t MPU_addr = 0x68;
int16_t AcX=0, AcY=0, AcZ=0, GyX=0, GyY=0, GyZ=0;

// ================== Fall Detection ==================
enum FallState {IDLE, TRIGGER1, TRIGGER2, TRIGGER3};
FallState fallState = IDLE;
byte counter = 0;
bool fallDetected = false;

// ================== Buzzer ==================
#define buzzer D6   // Buzzer connected to GPIO12
unsigned long lastSMS = 0;
const unsigned long smsCooldown = 30000; // 30 seconds cooldown

// ================== Function Prototypes ==================
bool readMPU();
void sendSMS();
void connectWiFi();

// ================== Setup ==================
void setup() {
  Serial.begin(115200);
  Wire.begin(D2, D1); // SDA, SCL

  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);

  // Initialize MPU6050
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  if (Wire.endTransmission(true) != 0) {
    Serial.println("❌ MPU6050 not found! Check wiring.");
    while(1);
  }
  Serial.println("✅ MPU6050 Initialized");

  connectWiFi();
}

// ================== Connect Wi-Fi ==================
void connectWiFi() {
  Serial.printf("🔌 Connecting to WiFi SSID: %s\n", ssid);
  WiFi.begin(ssid, password);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 30000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected!");
    Serial.print("📡 IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi failed! Retrying in 5s...");
    delay(5000);
    connectWiFi();
  }
}

// ================== Main Loop ==================
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi(); // Reconnect if disconnected
  }

  if (!readMPU()) {
    Serial.println("❌ Failed to read MPU6050");
    delay(100);
    return;
  }

  // Convert raw readings
  float ax = (AcX - 2050)/16384.0;
  float ay = (AcY - 77)/16384.0;
  float az = (AcZ - 1947)/16384.0;
  float gx = (GyX + 270)/131.07;
  float gy = (GyY - 351)/131.07;
  float gz = (GyZ + 136)/131.07;

  float acc = sqrt(ax*ax + ay*ay + az*az) * 10;
  float angleChange = sqrt(gx*gx + gy*gy + gz*gz);

  // -------- Fall Detection Logic --------
  switch(fallState) {
    case IDLE:
      if (acc <= 3) { 
        fallState = TRIGGER1;
        counter = 0;
        Serial.println("⚠ TRIGGER 1 ACTIVATED");
      }
      break;

    case TRIGGER1:
      counter++;
      if (acc >= 8) { 
        fallState = TRIGGER2;
        counter = 0;
        Serial.println("⚠ TRIGGER 2 ACTIVATED");
      } else if (counter >= 5) {
        fallState = IDLE;
        Serial.println("TRIGGER 1 DEACTIVATED");
      }
      break;

    case TRIGGER2:
      counter++;
      if (angleChange >= 15 && angleChange <= 300) {
        fallState = TRIGGER3;
        counter = 0;
        Serial.println("⚠ TRIGGER 3 ACTIVATED");
      } else if (counter >= 5) {
        fallState = IDLE;
        Serial.println("TRIGGER 2 DEACTIVATED");
      }
      break;

    case TRIGGER3:
      Serial.println("🚨 FALL DETECTED!");
      fallDetected = true;
      fallState = IDLE;
      counter = 0;
      break;
  }

  // If fall detected, trigger buzzer + send SMS
  if (fallDetected && (millis() - lastSMS > smsCooldown)) {
    Serial.println("🚨 Sending Alert...");
    digitalWrite(buzzer, HIGH);   // Turn buzzer ON
    delay(3000);                  // Sound duration
    digitalWrite(buzzer, LOW);    // Turn buzzer OFF
    sendSMS();
    lastSMS = millis();
    fallDetected = false;
  }

  delay(100);
}

// ================== Read MPU6050 ==================
bool readMPU() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;

  if (Wire.requestFrom((uint8_t)MPU_addr, (size_t)14, true) != 14) return false;

  AcX = Wire.read()<<8 | Wire.read();
  AcY = Wire.read()<<8 | Wire.read();
  AcZ = Wire.read()<<8 | Wire.read();
  Wire.read()<<8 | Wire.read(); // Skip temperature
  GyX = Wire.read()<<8 | Wire.read();
  GyY = Wire.read()<<8 | Wire.read();
  GyZ = Wire.read()<<8 | Wire.read();

  return true;
}

// ================== Send SMS via Twilio ==================
void sendSMS() {
  WiFiClientSecure client;
  client.setInsecure(); // Disable SSL certificate validation

  Serial.println("🌐 Connecting to Twilio...");
  if (!client.connect("api.twilio.com", 443)) {
    Serial.println("❌ Connection to Twilio failed!");
    return;
  }
  Serial.println("✅ Connected to Twilio");

  String url = "/2010-04-01/Accounts/" + accountSID + "/Messages.json";
  String body = "To=" + toNumber + "&From=" + fromNumber + "&Body=🚨 FALL DETECTED! Please check immediately.";
  String auth = accountSID + ":" + authToken;
  String encodedAuth = base64::encode(auth);

  client.println("POST " + url + " HTTP/1.1");
  client.println("Host: api.twilio.com");
  client.println("Authorization: Basic " + encodedAuth);
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.print("Content-Length: ");
  client.println(body.length());
  client.println();
  client.print(body);

  Serial.println("⏳ Waiting for Twilio response...");

  unsigned long timeout = millis() + 8000;
  while (client.connected() && millis() < timeout) {
    while (client.available()) {
      String line = client.readStringUntil('\n');
      Serial.println(line);
    }
  }

  client.stop();
  Serial.println("📩 SMS request sent!");
}