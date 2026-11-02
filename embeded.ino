/**
 * ==============================================================================
 * PRAKTIKUM EMBEDDED SYSTEM - STUDI KASUS SISTEM MONITORING LINGKUNGAN IOT
 * ==============================================================================
 * Deskripsi:
 * Firmware ESP32 non-blocking untuk memantau suhu dan kelembaban (DHT22),
 * mengukur status jaringan (RSSI), serta mengendalikan aktuator (LED & Buzzer)
 * secara real-time berdasarkan logika Multi-Condition Alert.
 *
 * Kriteria Multi-Condition Alert (Studi Kasus VII):
 * - Alarm aktif jika: Suhu > 38.0 °C  ATAU  Kelembaban < 20.0 %
 * - LED Merah ON & Buzzer aktif (1 kHz) saat kondisi kritis
 * - LED Hijau ON & Buzzer OFF saat kondisi normal
 * - Menampilkan indikasi teks pembeda spesifik di Serial Monitor untuk setiap pemicu
 * ==============================================================================
 */

#include <WiFi.h>
#include "DHTesp.h"

// ---------------------- DEFINISI PIN PERIFERAL ----------------------
const int DHT_PIN    = 15; // Pin Data DHT22
const int LED_NORMAL = 2;  // LED Hijau (Indikator Normal)
const int LED_ALARM  = 4;  // LED Merah (Indikator Kritis/Alarm)
const int BUZZER_PIN = 5;  // Piezo Buzzer (Indikator Audio Alarm)

// ---------------------- AMBANG BATAS KRITIS (THRESHOLD) -------------
const float AMBANG_SUHU_KRITIS       = 38.0; // Batas atas suhu (°C)
const float AMBANG_KELEMBABAN_KRITIS = 20.0; // Batas bawah kelembaban (% RH)

// ---------------------- KONFIGURASI WI-FI VIRTUAL WOKWI ------------
const char* ssid     = "Wokwi-GUEST";
const char* password = "";

DHTesp dhtSensor;

// ---------------------- PENJADWALAN NON-BLOCKING (millis) ----------
unsigned long waktuSebelumnya = 0;
const unsigned long intervalBaca = 2000; // Interval pembacaan sensor: 2000 ms (2 detik)

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n========================================================");
  Serial.println("  SISTEM MONITORING LINGKUNGAN BERBASIS IOT (ESP32)");
  Serial.println("  Modul Praktikum Embedded System - Mode Multi-Kondisi");
  Serial.println("========================================================");

  // Inisialisasi Pin Output Aktuator
  pinMode(LED_NORMAL, OUTPUT);
  pinMode(LED_ALARM, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Status awal aktuator: Safe State (Mati)
  digitalWrite(LED_NORMAL, LOW);
  digitalWrite(LED_ALARM, LOW);
  noTone(BUZZER_PIN);

  // Inisialisasi Sensor DHT22
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  Serial.println("[Init] Sensor DHT22 terkonfigurasi pada GPIO 15.");

  // Inisialisasi Wi-Fi Non-blocking dengan Timeout
  Serial.print("[Wi-Fi] Menghubungkan ke SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int attempts = 0;
  const int maxAttempts = 10;
  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(200);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[Wi-Fi Connected!] Berhasil terhubung ke jaringan.");
    Serial.print("[Wi-Fi Info] Alamat IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[Wi-Fi Timeout] Berpindah ke Mode Stand-alone / Offline.");
    Serial.println("[Info] Sistem tetap beroperasi penuh tanpa gateway internet.");
  }

  Serial.println("--------------------------------------------------------");
  Serial.println("Sistem Siap! Memulai loop pembacaan non-blocking...\n");
}

void loop() {
  unsigned long waktuSekarang = millis();

  // Pola Penjadwalan Waktu Non-blocking (Menggantikan delay())
  if (waktuSekarang - waktuSebelumnya >= intervalBaca) {
    waktuSebelumnya = waktuSekarang;

    // 1. Akuisisi Data dari Sensor DHT22
    TempAndHumidity data = dhtSensor.getTempAndHumidity();

    // 2. Validasi Integritas Data Sensor
    if (isnan(data.temperature) || isnan(data.humidity)) {
      Serial.println("[ERROR] Gagal membaca data dari sensor DHT22! Cek wiring koneksi sensor.");
      return;
    }

    // 3. Baca Nilai RSSI (Real jika terhubung, Mock/Simulasi jika offline)
    int rssi = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : -65;

    // 4. Tampilkan Log Telemetri Terformat
    Serial.printf("[Telemetri] Suhu: %5.2f °C | Kelembaban: %5.2f %% | RSSI: %d dBm\n",
                  data.temperature, data.humidity, rssi);

    // 5. Evaluasi Kondisi Kritis (Multi-Condition Alert)
    bool isSuhuKritis       = (data.temperature > AMBANG_SUHU_KRITIS);
    bool isKelembabanKritis = (data.humidity < AMBANG_KELEMBABAN_KRITIS);

    if (isSuhuKritis || isKelembabanKritis) {
      // --- KONDISI KRITIS (ALARM TERPICU) ---
      digitalWrite(LED_ALARM, HIGH); // Nyalakan LED Merah
      digitalWrite(LED_NORMAL, LOW); // Matikan LED Hijau
      tone(BUZZER_PIN, 1000);        // Bunyikan Buzzer dengan frekuensi 1 kHz

      // Indikasi teks pembeda spesifik di Serial Monitor
      if (isSuhuKritis && isKelembabanKritis) {
        Serial.printf(">>> [ALARM KRITIS - GANDA] Suhu Terlalu Tinggi (%.2f °C > %.1f °C) DAN Kelembaban Terlalu Kering (%.2f %% < %.1f %%)! <<<\n",
                      data.temperature, AMBANG_SUHU_KRITIS, data.humidity, AMBANG_KELEMBABAN_KRITIS);
      } else if (isSuhuKritis) {
        Serial.printf(">>> [ALARM KRITIS - SUHU] Suhu Melebihi Batas Aman! Nilai: %.2f °C (Ambang: %.1f °C) <<<\n",
                      data.temperature, AMBANG_SUHU_KRITIS);
      } else {
        Serial.printf(">>> [ALARM KRITIS - KELEMBABAN] Kelembaban Udara Terlalu Kering! Nilai: %.2f %% (Ambang: %.1f %%) <<<\n",
                      data.humidity, AMBANG_KELEMBABAN_KRITIS);
      }
    } else {
      // --- KONDISI NORMAL / AMAN ---
      digitalWrite(LED_ALARM, LOW);   // Matikan LED Merah
      digitalWrite(LED_NORMAL, HIGH); // Nyalakan LED Hijau
      noTone(BUZZER_PIN);             // Matikan Buzzer
      Serial.println("    [Status: NORMAL] Lingkungan Terkendali (Suhu & Kelembaban Aman).");
    }

    Serial.println("--------------------------------------------------------------------------------");
  }

  // -----------------------------------------------------------------------------
  // Catatan: Karena tidak menggunakan fungsi delay() yang bersifat blocking,
  // mikroprosesor dapat mengeksekusi background task lain (misal: OTA update,
  // penanganan paket jaringan, atau pembacaan tombol) secara kontinu di sini.
  // -----------------------------------------------------------------------------
}
