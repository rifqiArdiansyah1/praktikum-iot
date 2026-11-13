/**
 * ==============================================================================
 * UTS PRAKTIKUM EMBEDDED SYSTEM - SOAL 5
 * ==============================================================================
 * Judul     : Telemetri Sensor Cahaya & Kontrol Jarak Jauh Relay via Protokol MQTT
 * Komponen  : ESP32 DevKit, 1x Modul Sensor LDR (GPIO 34), 1x Modul Relay + Lampu (GPIO 19)
 * Pinout    : 
 *   - LDR AO              -> GPIO 34 (ADC1_CH6)
 *   - Relay IN (Control)  -> GPIO 19
 *
 * Konfigurasi Jaringan & Broker:
 *   - Wi-Fi SSID       : "Wokwi-GUEST" (Simulasi Wokwi)
 *   - MQTT Broker      : "broker.emqx.io" (Port 1883)
 *   - Topik Publish    : unisda/informatika/[NIM]/cahaya  (Setiap 5 detik)
 *   - Topik Subscribe  : unisda/informatika/[NIM]/lampu   (Menerima "ON" / "OFF")
 *
 * Logika Kerja:
 *   a. Publish: Mengakuisisi data intensitas kegelapan sensor LDR dan mengirimkannya
 *      ke broker MQTT setiap 5 detik dalam format payload JSON.
 *   b. Subscribe: Mendengarkan perintah kendali dari topik lampu.
 *      - Pesan "ON"  -> Aktifkan Relay (Lampu Menyala)
 *      - Pesan "OFF" -> Nonaktifkan Relay (Lampu Padam)
 * ==============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ---------------------- KONFIGURASI MAHASISWA ----------------------
// Silakan sesuaikan nilai NIM di bawah ini dengan NIM Anda masing-masing
const char* NIM = "rifqi"; 

// Definisi Pin Periferal
const int PIN_LDR   = 34; // Sensor Cahaya LDR (Analog ADC)
const int PIN_RELAY = 19; // Modul Relay Beban Lampu

// Konfigurasi Logika Modul Relay (Wokwi default: Active LOW)
const bool IS_ACTIVE_LOW = true; 
const int RELAY_ON  = IS_ACTIVE_LOW ? LOW  : HIGH;
const int RELAY_OFF = IS_ACTIVE_LOW ? HIGH : LOW;

// Konfigurasi Wi-Fi & MQTT
const char* ssid        = "Wokwi-GUEST";
const char* password    = "";
const char* mqtt_server = "broker.emqx.io";
const int   mqtt_port   = 1883;

// Objek Jaringan
WiFiClient espClient;
PubSubClient client(espClient);

// Variabel Topik MQTT Dinamis
String topicPublishCahaya;
String topicSubscribeLampu;

// Penjadwalan Non-blocking (Publish setiap 5 detik)
unsigned long lastPublishTime = 0;
const unsigned long INTERVAL_PUBLISH = 5000;

// Callback: Menangani pesan yang masuk dari broker pada topik subscribe
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();

  Serial.println("\n--------------------------------------------------");
  Serial.printf(">>> [MQTT PESAN MASUK] Topik: %s\n", topic);
  Serial.printf(">>> Isi Pesan: \"%s\"\n", message.c_str());

  // Logika Aktuasi Relay berdasarkan Perintah
  if (message.equalsIgnoreCase("ON")) {
    digitalWrite(PIN_RELAY, RELAY_ON);
    Serial.println(">>> [STATUS RELAY] Perintah 'ON' Diterima -> RELAY AKTIF (Lampu MENYALA) <<<");
  } else if (message.equalsIgnoreCase("OFF")) {
    digitalWrite(PIN_RELAY, RELAY_OFF);
    Serial.println(">>> [STATUS RELAY] Perintah 'OFF' Diterima -> RELAY NONAKTIF (Lampu PADAM) <<<");
  } else {
    Serial.println(">>> [STATUS RELAY] Perintah tidak valid! Gunakan pesan 'ON' atau 'OFF'.");
  }
  Serial.println("--------------------------------------------------\n");
}

// Fungsi Reconnect MQTT Broker
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("[MQTT] Menghubungkan ke broker: ");
    Serial.print(mqtt_server);
    Serial.print(" ... ");

    String clientId = "ESP32-UTS-" + String(NIM) + "-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("TERHUBUNG!");

      // Berlangganan (Subscribe) ke Topik Lampu
      client.subscribe(topicSubscribeLampu.c_str());
      Serial.printf("[MQTT Subscribe] Berhasil berlangganan ke topik: %s\n\n", topicSubscribeLampu.c_str());
    } else {
      Serial.print("Gagal, status rc=");
      Serial.print(client.state());
      Serial.println(" | Mencoba kembali dalam 3 detik...");
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n========================================================");
  Serial.println("UTS EMBEDDED SYSTEM - SOAL 5: TELEMETRI & KONTROL MQTT");
  Serial.println("========================================================");

  // Inisialisasi Pinout
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, RELAY_OFF); // Kondisi awal: Lampu mati
  analogReadResolution(12);

  // Buat String Topik Berdasarkan NIM
  topicPublishCahaya  = "unisda/informatika/" + String(NIM) + "/cahaya";
  topicSubscribeLampu = "unisda/informatika/" + String(NIM) + "/lampu";

  Serial.printf("[Info NIM] NIM Mahasiswa: %s\n", NIM);
  Serial.printf("[Topik Publish]   : %s\n", topicPublishCahaya.c_str());
  Serial.printf("[Topik Subscribe] : %s\n\n", topicSubscribeLampu.c_str());

  // Koneksi Wi-Fi
  Serial.print("[Wi-Fi] Menghubungkan ke: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\n[Wi-Fi Connected!] IP Address: " + WiFi.localIP().toString());

  // Konfigurasi Klien MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
  client.setBufferSize(512); // Perbesar buffer untuk payload JSON
}

void loop() {
  // Jaga konektivitas MQTT tetap aktif
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  unsigned long currentMillis = millis();

  // Kirim data pembacaan sensor cahaya setiap 5 detik (Non-blocking)
  if (currentMillis - lastPublishTime >= INTERVAL_PUBLISH) {
    lastPublishTime = currentMillis;

    // 1. Baca Nilai ADC Mentah Sensor LDR (0 - 4095)
    int rawADC = analogRead(PIN_LDR);

    // 2. Konversi ke Persentase Kegelapan (0% - 100%)
    // Sensor Wokwi LDR: nilai ADC tinggi saat gelap, rendah saat terang
    float darknessPercentage = (rawADC / 4095.0) * 100.0;
    darknessPercentage = constrain(darknessPercentage, 0.0, 100.0);

    // Identifikasi Kategori Cahaya
    String statusCahaya;
    if (darknessPercentage < 25.0) statusCahaya = "Sangat Terang";
    else if (darknessPercentage < 50.0) statusCahaya = "Terang";
    else if (darknessPercentage < 75.0) statusCahaya = "Redup";
    else statusCahaya = "Gelap";

    // 3. Susun Payload JSON (Kompatibel ArduinoJson v6 & v7)
    #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 7
      JsonDocument doc;
    #else
      StaticJsonDocument<256> doc;
    #endif

    doc["nim"]              = NIM;
    doc["uptime_sec"]       = currentMillis / 1000;
    doc["raw_adc"]          = rawADC;
    doc["kegelapan_pct"]    = serialized(String(darknessPercentage, 2));
    doc["status_cahaya"]    = statusCahaya;

    String jsonString;
    serializeJson(doc, jsonString);

    // 4. Publish ke Broker MQTT
    bool success = client.publish(topicPublishCahaya.c_str(), jsonString.c_str());

    // 5. Tampilkan Log Telemetri
    Serial.println("--------------------------------------------------");
    if (success) {
      Serial.printf(">>> [MQTT PUBLISH SUKSES] Topik: %s\n", topicPublishCahaya.c_str());
      Serial.printf(">>> Payload JSON: %s\n", jsonString.c_str());
    } else {
      Serial.println(">>> [MQTT PUBLISH GAGAL] Periksa koneksi jaringan atau ukuran buffer!");
    }
    Serial.println("--------------------------------------------------");
  }
}
