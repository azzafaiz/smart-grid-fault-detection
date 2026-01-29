#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>

// ---------- Namespaces ----------
using namespace firebase;
using namespace realtime_database;

// ---------- WiFi CONFIG ----------
#define WIFI_SSID "vivo v30 5g"
#define WIFI_PASSWORD "01234567"

// ---------- FIREBASE CONFIG ----------
#define Web_API_KEY "AIzaSyDEumQF9TYkAIHhF7YAlAC3auRaSnIkmcg"
#define DATABASE_URL "https://smartplug-acd42-default-rtdb.asia-southeast1.firebasedatabase.app"
#define USER_EMAIL "samuel@gmail.com"
#define USER_PASS "123456"

#define POLE_DATA_PATH "plug_1/data"
#define AC_VOLTAGE 220.0

// ---------- SENSOR & RELAY PINS ----------
#define CURRENT_PIN A0
#define PIR_PIN D4          // GPIO2
#define RELAY_PIN D1        // GPIO5 (safe for boot)
#define TOGGLE_BTN_PIN D5   // GPIO14

// ---------- Calibration Constants ----------
const float Vref = 3.3;
const int ADCmax = 1023;
const float sensitivity = 185.0;
const float zeroCurrentVoltage = Vref / 2.0;
const int SAMPLES = 800;
const float CALIBRATION_FACTOR = 0.041;
const float HIGH_CURRENT_THRESHOLD = 0.5; // Adjust as needed (A)

// ---------- Smart Mode Settings ----------
bool smartMode = true;           // true = Smart Mode, false = Manual/Web Mode
bool relayState = true;
bool motionDetected = false;
unsigned long lastMotionTime = 0;
const unsigned long motionTimeout = 300000; // 5 min
bool isDaytime = true;           // placeholder
bool relayOverride = false;      // Remote override flag

// ---------- Firebase & Networking ----------
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);
RealtimeDatabase Database;

// ---------- Timers ----------
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 3000;

// ---------- Function Prototypes ----------
float calculate_rms_current();
void processData(AsyncResult &aResult);
void updateRelayState(bool state);
void relayDataCallback(AsyncResult &aResult);
void overrideCallback(AsyncResult &aResult);

// ---------- RMS Current Measurement ----------
float calculate_rms_current() {
  long sumSq = 0;
  float zeroOffset = (zeroCurrentVoltage * ADCmax) / Vref;

  for (int i = 0; i < SAMPLES; i++) {
    int raw = analogRead(CURRENT_PIN);
    int diff = raw - zeroOffset;
    sumSq += (long)diff * diff;
  }

  float avg = (float)sumSq / SAMPLES;
  float rms = sqrt(avg);
  float current = rms * CALIBRATION_FACTOR;
  return current < 0 ? 0 : current;
}

// ---------- Firebase Error Handler ----------
void processData(AsyncResult &aResult) {
  if (aResult.isError()) {
    Serial.printf("Firebase Error [%d]: %s\n",
                  aResult.error().code(),
                  aResult.error().message().c_str());
  }
}

// ---------- Relay State Update ----------
void updateRelayState(bool state) {
  digitalWrite(RELAY_PIN, state ? HIGH : LOW);
  relayState = state;

  FirebaseJson json;
  json.set("relay_state", relayState);
  Database.set(aClient, String(POLE_DATA_PATH), json, processData, "Relay_Update");
}

// ---------- Firebase callback for web relay control ----------
void relayDataCallback(AsyncResult &aResult) {
  if (!aResult.isResult() || !aResult.available()) return;
  if (aResult.isError()) {
    Serial.printf("Firebase GET Error: %s (%d)\n",
                  aResult.error().message().c_str(),
                  aResult.error().code());
    return;
  }

  String payload = aResult.c_str();
  payload.trim();

  if (!smartMode && calculate_rms_current() <= HIGH_CURRENT_THRESHOLD) {
    bool webRelayState = (payload == "true" || payload == "1");
    if (relayState != webRelayState) {
      updateRelayState(webRelayState);
      Serial.printf("Relay updated by web/manual: %s\n", relayState ? "ON" : "OFF");
    }
  }
}

// ---------- Firebase callback for remote override ----------
void overrideCallback(AsyncResult &aResult) {
  if (!aResult.isResult() || !aResult.available()) return;
  if (aResult.isError()) {
    Serial.printf("Firebase OVERRIDE Error: %s (%d)\n",
                  aResult.error().message().c_str(),
                  aResult.error().code());
    return;
  }

  String payload = aResult.c_str();
  payload.trim();
  relayOverride = (payload == "true" || payload == "1");
  Serial.printf("Relay override flag: %s\n", relayOverride ? "ON" : "OFF");
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);

  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // initially ON
  pinMode(TOGGLE_BTN_PIN, INPUT_PULLUP); // manual toggle button

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nWi-Fi Connected!");
  Serial.println(WiFi.localIP());

  ssl_client.setInsecure();

  initializeApp(aClient, app, getAuth(user_auth), processData, "auth");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
  Database.setRef(DATABASE_URL);
  Serial.println("Firebase Ready...");

  aClient.keepAlive(10, 10, 1);
}

// ---------- Loop ----------
void loop() {
  app.loop();
  if (!app.ready()) return;

  unsigned long now = millis();
  float currentA = calculate_rms_current();
  float powerW = currentA * AC_VOLTAGE;

  // --- WiFi Auto Reconnect ---
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    delay(500);
  }

  // --- Motion Detection ---
  if (digitalRead(PIR_PIN) == HIGH) {
    motionDetected = true;
    lastMotionTime = now;
  } else if (now - lastMotionTime > motionTimeout) {
    motionDetected = false;
  }

  // --- Manual Toggle Button ---
  static bool lastBtnState = HIGH;
  bool btnState = digitalRead(TOGGLE_BTN_PIN);
  if (lastBtnState == HIGH && btnState == LOW) {
    smartMode = !smartMode;  // toggle mode
    Serial.printf("Smart Mode toggled: %s\n", smartMode ? "ON" : "OFF");
    Database.set<bool>(aClient, String(POLE_DATA_PATH) + "/smart_mode", smartMode, processData, "Mode");
    delay(150); // debounce
  }
  lastBtnState = btnState;

  // --- Relay Control Logic ---
  if (currentA > HIGH_CURRENT_THRESHOLD) {
    if (relayState != false) {
      updateRelayState(false);
      Serial.println("Relay OFF due to high current!");
    }
  } else {
    if (smartMode) {
      if (relayOverride) {
        updateRelayState(true);
      } else {
        if (!motionDetected && isDaytime && currentA > 0.2)
          updateRelayState(false);
        else
          updateRelayState(true);
      }
    } else {
      static unsigned long lastRelayCheck = 0;
      const unsigned long relayCheckInterval = 2000;
      if (now - lastRelayCheck >= relayCheckInterval) {
        lastRelayCheck = now;
        Database.get(aClient, String(POLE_DATA_PATH) + "/relay_state", relayDataCallback, false, "WEB_RELAY");
      }
    }
  }

  // --- Remote override check ---
  static unsigned long lastOverrideCheck = 0;
  const unsigned long overrideCheckInterval = 2000;
  if (now - lastOverrideCheck >= overrideCheckInterval) {
    lastOverrideCheck = now;
    Database.get(aClient, String(POLE_DATA_PATH) + "/relay_override", overrideCallback, false, "OVERRIDE");
  }

  // --- Firebase Upload ---
  if (now - lastSendTime >= sendInterval) {
    lastSendTime = now;

    FirebaseJson json;
    json.set("current_A", currentA);
    json.set("power_W", powerW);
    json.set("voltage_V", AC_VOLTAGE);
    json.set("motion", motionDetected);
    json.set("smart_mode", smartMode);
    json.set("relay_state", relayState);
    json.set("relay_override", relayOverride);
    json.set("timestamp", (int)now);

    Database.set(aClient, String(POLE_DATA_PATH), json, processData, "Bulk_Update");

    Serial.printf("Upload → %.2f A | %.2f W | Relay: %s | Motion: %s | Mode: %s | Override: %s\n",
                  currentA, powerW,
                  relayState ? "ON" : "OFF",
                  motionDetected ? "YES" : "NO",
                  smartMode ? "Smart" : "Manual",
                  relayOverride ? "ON" : "OFF");
  }
}
