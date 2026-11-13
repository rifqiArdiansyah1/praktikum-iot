/**
 * ==============================================================================
 * UTS PRAKTIKUM EMBEDDED SYSTEM - SOAL 2
 * ==============================================================================
 * Judul     : Pembacaan Sensor Analog LDR dan Pengolahan Data ADC 12-Bit
 * Komponen  : ESP32 DevKit, 1x Modul Sensor LDR (Wokwi Photoresistor Sensor)
 * Pinout    : 
 *   - LDR VCC   -> 3V3 ESP32
 *   - LDR GND   -> GND ESP32
 *   - LDR AO    -> GPIO 34 ESP32 (ADC1_CH6 - Input Only)
 *
 * Logika Pengolahan ADC:
 *   1. Nilai ADC mentah ESP32 beresolusi 12-bit dengan rentang 0 hingga 4095.
 *   2. Karakteristik Modul Sensor LDR Wokwi (wokwi-photoresistor-sensor):
 *      - Terang (High Lux) -> Resistansi LDR kecil -> Tegangan AO rendah (~0.04V - 0.2V) -> ADC mendekati 0.
 *      - Gelap (Low Lux)   -> Resistansi LDR besar -> Tegangan AO tinggi (~3.3V)        -> ADC mendekati 4095.
 *   3. Konversi Persentase Intensitas Kegelapan (0% - 100%):
 *      - Persentase Kegelapan (%) = (rawADC / 4095.0) * 100.0%
 *        * rawADC = 4095 -> Kegelapan 100.0% (Kondisi Gelap Gulita)
 *        * rawADC = 0    -> Kegelapan 0.0%   (Kondisi Sangat Terang)
 *      (Catatan: Disediakan flag IS_WOKWI_MODULE untuk menyesuaikan jika menggunakan rangkaian divider terbalik)
 *   4. Menampilkan data telemetri ke Serial Monitor setiap 1 detik secara non-blocking (millis()).
 * ==============================================================================
 */

#include <Arduino.h>

// Definisi Pin Analog LDR
const int PIN_LDR = 34; // GPIO 34 (ADC1 Channel 6)

// Konfigurasi Rangkaian Voltage Divider:
// true  : Modul Sensor LDR Wokwi (Nilai ADC naik seiring semakin gelapnya ruangan)
// false : Rangkaian divider konvensional LDR ke 3.3V dan R 10k ke GND (Nilai ADC turun saat gelap)
const bool IS_WOKWI_MODULE = true;

// Penjadwalan Waktu Non-blocking
unsigned long lastReadTime = 0;
const unsigned long INTERVAL_BACA = 1000; // Baca dan tampilkan setiap 1000 ms (1 detik)

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n========================================================");
  Serial.println("UTS EMBEDDED SYSTEM - SOAL 2: PEMBACAAN ADC SENSOR LDR");
  Serial.println("========================================================");

  // Konfigurasi ADC ESP32: Resolusi 12-bit (0 - 4095)
  analogReadResolution(12);

  // Set atenuasi 11dB untuk membaca tegangan input hingga ~3.3V (standar ESP32)
  analogSetAttenuation(ADC_11db);

  Serial.println("[Info] Pin GPIO 34 terkonfigurasi sebagai input ADC 12-bit.");
  Serial.println("Memulai pembacaan data setiap 1 detik...\n");
  Serial.println("------------------------------------------------------------------");
  Serial.printf("%-12s | %-12s | %-15s | %-12s\n", "Waktu (detik)", "ADC Mentah", "Tegangan (V)", "Kegelapan (%)");
  Serial.println("------------------------------------------------------------------");
}

void loop() {
  unsigned long currentMillis = millis();

  // Eksekusi setiap 1 detik secara non-blocking
  if (currentMillis - lastReadTime >= INTERVAL_BACA) {
    lastReadTime = currentMillis;

    // a. Baca nilai mentah ADC 12-bit (Rentang: 0 - 4095)
    int rawADC = analogRead(PIN_LDR);

    // Hitung estimasi tegangan analog (0.0 V - 3.3 V)
    float voltage = (rawADC / 4095.0) * 3.3;

    // b. Konversi ke persentase intensitas kegelapan (0% - 100%)
    float darknessPercentage;
    if (IS_WOKWI_MODULE) {
      // Pada sensor Wokwi: ADC tinggi saat gelap, ADC rendah saat terang
      darknessPercentage = (rawADC / 4095.0) * 100.0;
    } else {
      // Pada divider LDR pull-up ke 3.3V
      darknessPercentage = ((4095.0 - rawADC) / 4095.0) * 100.0;
    }

    // Pastikan nilai berada di rentang 0.0% - 100.0%
    darknessPercentage = constrain(darknessPercentage, 0.0, 100.0);

    // Identifikasi deskripsi kondisi pencahayaan
    String statusCahaya;
    if (darknessPercentage < 25.0) {
      statusCahaya = "Sangat Terang";
    } else if (darknessPercentage < 50.0) {
      statusCahaya = "Terang";
    } else if (darknessPercentage < 75.0) {
      statusCahaya = "Redup / Temaram";
    } else {
      statusCahaya = "Gelap Gulita";
    }

    // Tampilkan data ke Serial Monitor
    Serial.printf("%-12lu | %-12d | %-15.2f | %6.2f %% (%s)\n",
                  currentMillis / 1000, rawADC, voltage, darknessPercentage, statusCahaya.c_str());
  }
}
