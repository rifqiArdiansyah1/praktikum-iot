/**
 * ==============================================================================
 * UTS PRAKTIKUM EMBEDDED SYSTEM - PROYEK TERPADU (ALL-IN-ONE SYSTEM)
 * ==============================================================================
 * Program ini mengintegrasikan seluruh 5 modul soal UTS dalam 1 firmware
 * mikrokontroler ESP32 tanpa konflik pinout, dijalankan secara fully non-blocking:
 * 
 * 1. Soal 1 : Kontrol Kecerahan LED PWM (GPIO 16) dengan Push Button (GPIO 4) + Debouncing
 * 2. Soal 2 : Akuisisi Sensor Cahaya LDR (GPIO 34 ADC 12-bit) & Konversi % Kegelapan (tiap 1s)
 * 3. Soal 3 : Siklus Otomatis Relay (GPIO 19) ON 5s & OFF 3s (Active HIGH/LOW)
 * 4. Soal 4 : Palang Pintu Otomatis Motor Servo SG90 (GPIO 18) + HC-SR04 (Trig: 5, Echo: 17)
 * 5. Soal 5 : IoT Telemetri MQTT (Publish LDR tiap 5s & Subscribe Kontrol Lampu ON/OFF)
 * ==============================================================================
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// ---------------------- KONFIGURASI MAHASISWA ----------------------
const char* NIM = "rifqi"; // Sesuaikan dengan NIM Anda

// ---------------------- DEFINISI PINOUT SELURUH SISTEM --------------
const int PIN_LED_PWM     = 16; // Soal 1: LED PWM
const int PIN_BUTTON      = 4;  // Soal 1: Push Button
const int PIN_LDR         = 34; // Soal 2 & 5: Sensor Cahaya LDR (ADC1_CH6)
const int PIN_RELAY       = 19; // Soal 3 & 5: Modul Relay
const int PIN_SERVO       = 18; // Soal 4: Sinyal PWM Servo SG90
const int PIN_TRIG        = 5;  // Soal 4: HC-SR04 Trig
const int PIN_ECHO        = 17; // Soal 4: HC-SR04 Echo

// ---------------------- SOAL 1: VARIABEL PWM & BUTTON ---------------
const int DUTY_LEVELS[4] = { 0, (int)(255 * 0.30), (int)(255 * 0.70), 255 };
const char* LEVEL_NAMES[4] = { "0% (Padam)", "30% (Redup)", "70% (Sedang)", "100% (Maksimal)" };
int currentLedLevel = 0;
int buttonState = HIGH;
int lastRawButtonReading = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

void setLedPWM(int duty) {
  #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(PIN_LED_PWM, duty);
  #else
    ledcWrite(0, duty);
  #endif
}

// ---------------------- SOAL 2: VARIABEL LDR & ADC ------------------
unsigned long lastLdrPrintTime = 0;
const unsigned long INTERVAL_LDR_PRINT = 1000;
float currentDarknessPct = 0.0;
int currentRawADC = 0;

// ---------------------- SOAL 3: VARIABEL RELAY TIMING ---------------
const bool IS_ACTIVE_LOW = false;
const int RELAY_ON  = IS_ACTIVE_LOW ? LOW  : HIGH;
const int RELAY_OFF = IS_ACTIVE_LOW ? HIGH : LOW;

// Mode Kendali Relay: false = Mengikuti Siklus Timer Soal 3 (ON 5s / OFF 3s)
//                     true  = Diambil alih oleh Perintah Remote MQTT Soal 5
bool mqttManualOverride = false;
bool relayState = false;
unsigned long lastRelayToggleTime = 0;
const unsigned long RELAY_ON_DURATION  = 5000;
const unsigned long RELAY_OFF_DURATION = 3000;

// ---------------------- SOAL 4: VARIABEL SERVO & HC-SR04 ------------
Servo barrierServo;
const float AMBANG_JARAK = 10.0;
int servoAngle = -1;
unsigned long lastSonarTime = 0;
const unsigned long INTERVAL_SONAR = 200;

float getUltrasonicDistance() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  long duration = pulseIn(PIN_ECHO, HIGH, 30000);
  if (duration == 0) return 999.0;
  return (duration * 0.0343) / 2.0;
}

// ---------------------- SOAL 5: JARINGAN WI-FI & MQTT ---------------
const char* ssid        = "Wokwi-GUEST";
const char* password    = "";
const char* mqtt_server = "broker.emqx.io";
const int   mqtt_port   = 1883;

WiFiClient espClient;
PubSubClient mqttClient(espClient);
String topicPubCahaya;
String topicSubLampu;
unsigned long lastMqttPublishTime = 0;
const unsigned long INTERVAL_MQTT_PUB = 5000;

void callbackMQTT(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();

  Serial.println("\n--------------------------------------------------");
  Serial.printf(">>> [MQTT REMOTE COMMAND] Topik: %s | Pesan: %s\n", topic, msg.c_str());

  if (msg.equalsIgnoreCase("ON")) {
    mqttManualOverride = true; // Mengambil alih kontrol otomatis
    digitalWrite(PIN_RELAY, RELAY_ON);
    relayState = true;
    Serial.println(">>> [RELAY OVERRIDE] Diaktifkan secara Remote via MQTT (Lampu MENYALA)");
  } else if (msg.equalsIgnoreCase("OFF")) {
    mqttManualOverride = true;
    digitalWrite(PIN_RELAY, RELAY_OFF);
    relayState = false;
    Serial.println(">>> [RELAY OVERRIDE] Dinonaktifkan secara Remote via MQTT (Lampu PADAM)");
  } else if (msg.equalsIgnoreCase("AUTO")) {
    mqttManualOverride = false; // Kembalikan ke siklus otomatis 5s/3s Soal 3
    lastRelayToggleTime = millis();
    Serial.println(">>> [RELAY RESTORE] Mode Siklus Otomatis 5s ON / 3s OFF Dipulihkan");
  }
  Serial.println("--------------------------------------------------\n");
}

void reconnectBroker() {
  while (!mqttClient.connected()) {
    Serial.print("[MQTT] Menghubungkan ke broker EMQX... ");
    String clientId = "ESP32-UTS-ALL-" + String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("TERHUBUNG!");
      mqttClient.subscribe(topicSubLampu.c_str());
      Serial.printf("[MQTT Sub] Berlangganan: %s\n", topicSubLampu.c_str());
    } else {
      Serial.printf("Gagal (rc=%d), coba lagi 3 detik...\n", mqttClient.state());
      delay(3000);
    }
  }
}

// ==============================================================================
// SETUP SYSTEM
// ==============================================================================
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n========================================================");
  Serial.println("  UTS EMBEDDED SYSTEM - PROYEK TERPADU KESELURUHAN SOAL");
  Serial.println("========================================================");

  // 1. Setup Soal 1 (LED PWM & Button)
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(PIN_LED_PWM, 5000, 8);
  #else
    ledcSetup(0, 5000, 8);
    ledcAttachPin(PIN_LED_PWM, 0);
  #endif
  setLedPWM(DUTY_LEVELS[currentLedLevel]);

  // 2. Setup Soal 2 (ADC LDR)
  analogReadResolution(12);

  // 3. Setup Soal 3 (Relay)
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, RELAY_ON);
  relayState = true;
  lastRelayToggleTime = millis();

  // 4. Setup Soal 4 (Servo & HC-SR04)
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  ESP32PWM::allocateTimer(0);
  barrierServo.setPeriodHertz(50);
  barrierServo.attach(PIN_SERVO, 500, 2400);
  barrierServo.write(0);
  servoAngle = 0;

  // 5. Setup Soal 5 (Wi-Fi & MQTT)
  topicPubCahaya = "unisda/informatika/" + String(NIM) + "/cahaya";
  topicSubLampu  = "unisda/informatika/" + String(NIM) + "/lampu";

  Serial.print("[Wi-Fi] Menghubungkan ke Wi-Fi Wokwi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\n[Wi-Fi Online] IP: " + WiFi.localIP().toString());

  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(callbackMQTT);
  mqttClient.setBufferSize(512);

  Serial.println("\nSeluruh 5 subsistem telah aktif dan berjalan simultan!\n");
}

// ==============================================================================
// MAIN LOOP (NON-BLOCKING CONCURRENT TASKS)
// ==============================================================================
void loop() {
  if (!mqttClient.connected()) {
    reconnectBroker();
  }
  mqttClient.loop();

  unsigned long now = millis();

  // ---------------------------------------------------------------------------
  // TASK 1 (SOAL 1): DEBOUNCING PUSH BUTTON & PWM LED
  // ---------------------------------------------------------------------------
  int rawBtn = digitalRead(PIN_BUTTON);
  if (rawBtn != lastRawButtonReading) lastDebounceTime = now;
  if ((now - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (rawBtn != buttonState) {
      buttonState = rawBtn;
      if (buttonState == LOW) {
        currentLedLevel = (currentLedLevel + 1) % 4;
        setLedPWM(DUTY_LEVELS[currentLedLevel]);
        Serial.printf("[SOAL 1 - PWM LED] Kecerahan Diubah: %s (Duty: %d/255)\n",
                      LEVEL_NAMES[currentLedLevel], DUTY_LEVELS[currentLedLevel]);
      }
    }
  }
  lastRawButtonReading = rawBtn;

  // ---------------------------------------------------------------------------
  // TASK 2 (SOAL 2): PEMBACAAN ANALOG LDR SETIAP 1 DETIK
  // ---------------------------------------------------------------------------
  if (now - lastLdrPrintTime >= INTERVAL_LDR_PRINT) {
    lastLdrPrintTime = now;
    currentRawADC = analogRead(PIN_LDR);
    currentDarknessPct = constrain(((4095.0 - currentRawADC) / 4095.0) * 100.0, 0.0, 100.0);
    Serial.printf("[SOAL 2 - LDR ADC] Raw ADC: %4d | Kegelapan: %5.1f %%\n",
                  currentRawADC, currentDarknessPct);
  }

  // ---------------------------------------------------------------------------
  // TASK 3 (SOAL 3): SIKLUS RELAY OTOMATIS (5s ON / 3s OFF)
  // ---------------------------------------------------------------------------
  if (!mqttManualOverride) {
    if (relayState && (now - lastRelayToggleTime >= RELAY_ON_DURATION)) {
      digitalWrite(PIN_RELAY, RELAY_OFF);
      relayState = false;
      lastRelayToggleTime = now;
      Serial.println("[SOAL 3 - RELAY] Otomatis: MATI (3 Detik)...");
    } else if (!relayState && (now - lastRelayToggleTime >= RELAY_OFF_DURATION)) {
      digitalWrite(PIN_RELAY, RELAY_ON);
      relayState = true;
      lastRelayToggleTime = now;
      Serial.println("[SOAL 3 - RELAY] Otomatis: HIDUP (5 Detik)...");
    }
  }

  // ---------------------------------------------------------------------------
  // TASK 4 (SOAL 4): DETEKSI ULTRASONIK & PALANG PINTU SERVO
  // ---------------------------------------------------------------------------
  if (now - lastSonarTime >= INTERVAL_SONAR) {
    lastSonarTime = now;
    float dist = getUltrasonicDistance();
    if (dist < AMBANG_JARAK) {
      if (servoAngle != 90) {
        barrierServo.write(90);
        servoAngle = 90;
        Serial.printf("[SOAL 4 - PALANG] Terbuka (90°) | Objek Terdeteksi pada %.1f cm\n", dist);
      }
    } else {
      if (servoAngle != 0) {
        barrierServo.write(0);
        servoAngle = 0;
        Serial.printf("[SOAL 4 - PALANG] Tertutup (0°)  | Area Bersih (%.1f cm)\n", dist);
      }
    }
  }

  // ---------------------------------------------------------------------------
  // TASK 5 (SOAL 5): TELEMETRI MQTT CAHAYA SETIAP 5 DETIK
  // ---------------------------------------------------------------------------
  if (now - lastMqttPublishTime >= INTERVAL_MQTT_PUB) {
    lastMqttPublishTime = now;
    StaticJsonDocument<256> doc;
    doc["nim"]           = NIM;
    doc["uptime_sec"]    = now / 1000;
    doc["raw_adc"]       = currentRawADC;
    doc["kegelapan_pct"] = serialized(String(currentDarknessPct, 2));
    
    String jsonStr;
    serializeJson(doc, jsonStr);
    mqttClient.publish(topicPubCahaya.c_str(), jsonStr.c_str());
    Serial.printf("[SOAL 5 - MQTT PUB] Terkirim ke %s: %s\n", topicPubCahaya.c_str(), jsonStr.c_str());
  }
}
