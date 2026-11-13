/**
 * ==============================================================================
 * UTS PRAKTIKUM EMBEDDED SYSTEM - SOAL 3
 * ==============================================================================
 * Judul     : Pengendalian Sakelar Beban Daya Menggunakan Modul Relay
 * Komponen  : ESP32 DevKit, 1x Modul Relay (Wokwi Relay Module), 1x Lampu Beban (LED)
 * Pinout    : 
 *   - Relay VCC        -> 5V (atau 3V3) ESP32
 *   - Relay GND        -> GND ESP32
 *   - Relay IN (Signal)-> GPIO 19 ESP32
 *   - Terminal Relay   : 
 *     * COM (Common)        -> Sumber Daya (3V3 ESP32)
 *     * NO (Normally Open)  -> Beban Lampu (+) melalui Resistor 220 Ohm
 *
 * Pola Kerja Waktu:
 *   - Relay Aktif (Lampu Menyala) selama 5 detik (5000 ms).
 *   - Relay Nonaktif (Lampu Padam) selama 3 detik (3000 ms).
 *   - Berjalan secara siklus kontinu (looping) menggunakan non-blocking timer (millis()).
 *
 * Penjelasan Soal 3.b (Logika Pemicuan Relay - Active LOW vs Active HIGH):
 *   1. Modul Relay Active LOW (Bawaan Simulator Wokwi & Mayoritas Modul Relay Fisik):
 *      - Relay aktif saat pin kendali (IN) berlogika LOW (0V).
 *      - Rangkaian input optocoupler/transistor NPN internal terpicu saat pin IN ditarik
 *        ke Ground (sink current), mengalirkan arus ke koil elektromagnetik dan menutup
 *        sakelar terminal COM - NO (Lampu Menyala).
 *      - Saat pin IN berlogika HIGH (3.3V/5V), koil kehilangan daya dan sakelar kembali
 *        ke posisi NC (Lampu Padam).
 *   2. Modul Relay Active HIGH:
 *      - Relay aktif saat pin kendali (IN) berlogika HIGH (3.3V/5V), di mana arus keluar
 *        dari GPIO (source current) untuk menyalakan transistor PNP / driver koil.
 *   3. Penentuan dalam Kode ini:
 *      - Parameter `IS_ACTIVE_LOW = true` disetel agar sinkron dengan karakteristik
 *        bawaan modul relay Wokwi (wokwi-relay-module).
 * ==============================================================================
 */

#include <Arduino.h>

// Definisi Pin Relay
const int PIN_RELAY = 19;

// -----------------------------------------------------------------------------
// KONFIGURASI LOGIKA RELAY (Jawaban Soal 3.b)
// -----------------------------------------------------------------------------
// Modul relay Wokwi dan modul relay fisik pada umumnya berkarakteristik Active LOW.
const bool IS_ACTIVE_LOW = true; 

const int RELAY_ON  = IS_ACTIVE_LOW ? LOW  : HIGH;
const int RELAY_OFF = IS_ACTIVE_LOW ? HIGH : LOW;

// Definisi Interval Waktu (Non-blocking)
const unsigned long DURASI_MENYALA = 5000; // 5 Detik Relay Aktif (Lampu ON)
const unsigned long DURASI_PADAM   = 3000; // 3 Detik Relay Nonaktif (Lampu OFF)

// Variabel Penjadwalan State Machine
unsigned long lastStateChangeTime = 0;
bool isLampOn = false; // Status awal relay/lampu

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n========================================================");
  Serial.println("UTS EMBEDDED SYSTEM - SOAL 3: PENGENDALIAN SAKELAR RELAY");
  Serial.println("========================================================");

  // Inisialisasi Pin GPIO sebagai OUTPUT
  pinMode(PIN_RELAY, OUTPUT);

  // Tampilkan informasi logika pemicuan yang digunakan
  Serial.printf("[Konfigurasi Logika] Karakteristik Relay: %s\n", 
                IS_ACTIVE_LOW ? "Active LOW (LOW = ON / COM-NO, HIGH = OFF / COM-NC)" 
                              : "Active HIGH (HIGH = ON / COM-NO, LOW = OFF / COM-NC)");

  // Mulai siklus: Langsung aktifkan relay (Kondisi Awal: Lampu Menyala)
  digitalWrite(PIN_RELAY, RELAY_ON);
  isLampOn = true;
  lastStateChangeTime = millis();

  Serial.println("[SIKLUS 1] Relay DIAKTIFKAN -> Lampu MENYALA (Target: 5 Detik)...");
}

void loop() {
  unsigned long currentMillis = millis();

  // Evaluasi perpindahan status berdasarkan durasi masing-masing kondisi
  if (isLampOn) {
    // Jika sedang menyala dan durasi 5 detik telah terpenuhi
    if (currentMillis - lastStateChangeTime >= DURASI_MENYALA) {
      // Matikan relay
      digitalWrite(PIN_RELAY, RELAY_OFF);
      isLampOn = false;
      lastStateChangeTime = currentMillis;

      Serial.printf("[Waktu: %lu ms] Relay DINONAKTIFKAN -> Lampu PADAM (Target: 3 Detik)...\n", currentMillis);
    }
  } else {
    // Jika sedang padam dan durasi 3 detik telah terpenuhi
    if (currentMillis - lastStateChangeTime >= DURASI_PADAM) {
      // Nyalakan relay kembali
      digitalWrite(PIN_RELAY, RELAY_ON);
      isLampOn = true;
      lastStateChangeTime = currentMillis;

      Serial.printf("[Waktu: %lu ms] Relay DIAKTIFKAN KEMBALI -> Lampu MENYALA (Target: 5 Detik)...\n", currentMillis);
    }
  }

  // Pola non-blocking memungkinkan mikrokontroler mengeksekusi task lain di sini
}
