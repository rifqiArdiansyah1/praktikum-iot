/**
 * ==============================================================================
 * UTS PRAKTIKUM EMBEDDED SYSTEM - SOAL 2
 * ==============================================================================
 * Judul     : Pembacaan Sensor Analog LDR dan Pengolahan Data ADC 12-Bit
 * Komponen  : ESP32 DevKit, 1x LDR (Photoresistor), 1x Resistor 10 kOhm
 * Pinout    : 
 *   - LDR Pin 1        -> 3V3 ESP32
 *   - LDR Pin 2        -> Node Tengah (Terhubung ke GPIO 34 & Resistor 10k Pin 1)
 *   - Resistor 10k Pin 2 -> GND ESP32
 *   - Output Divider   -> GPIO 34 (ADC1_CH6 - Input Only)
 *
 * Logika Pengolahan ADC:
 *   1. Nilai ADC mentah ESP32 beresolusi 12-bit dengan rentang 0 hingga 4095.
 *   2. Karakteristik Rangkaian Voltage Divider (LDR sisi 3V3, Resistor 10k sisi GND):
 *      - Terang : Hambatan LDR turun -> Tegangan GPIO 34 naik -> Nilai ADC mendekati 4095.
 *      - Gelap  : Hambatan LDR naik  -> Tegangan GPIO 34 turun -> Nilai ADC mendekati 0.
 *   3. Konversi Persentase Kegelapan (0% - 100%):
 *      - Persentase Kegelapan (%) = ((4095 - rawADC) / 4095.0) * 100.0%
 *        * rawADC = 4095 -> Kegelapan 0.0% (Sangat Terang)
 *        * rawADC = 0    -> Kegelapan 100.0% (Gelap Gulita)
 *   4. Menampilkan data telemetri ke Serial Monitor setiap 1 detik secara non-blocking (millis()).
 * ==============================================================================
 */

// Definisi Pin Analog LDR
const int PIN_LDR = 34; // GPIO 34 (ADC1 Channel 6)

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
    // Pada rangkaian pembagi tegangan (LDR ke 3.3V, R 10k ke GND):
    // Nilai ADC tinggi = Terang, Nilai ADC rendah = Gelap.
    float darknessPercentage = ((4095.0 - rawADC) / 4095.0) * 100.0;

    // Pastikan nilai berada di batas aman 0.0% - 100.0%
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
