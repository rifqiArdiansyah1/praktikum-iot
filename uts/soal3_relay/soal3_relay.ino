/**
 * ==============================================================================
 * UTS PRAKTIKUM EMBEDDED SYSTEM - SOAL 3
 * ==============================================================================
 * Judul     : Pengendalian Sakelar Beban Daya Menggunakan Modul Relay
 * Komponen  : ESP32 DevKit, 1x Modul Relay 5V/3.3V, 1x Lampu Beban (Lightbulb/LED)
 * Pinout    : 
 *   - Relay VCC        -> Vin / 5V (atau 3V3) ESP32
 *   - Relay GND        -> GND ESP32
 *   - Relay IN (Signal)-> GPIO 19 ESP32
 *   - Terminal Relay   : 
 *     * COM (Common)        -> Sumber Daya Lampu
 *     * NO (Normally Open)  -> Beban Lampu (+)
 *
 * Pola Kerja Waktu:
 *   - Relay Aktif (Lampu Menyala) selama 5 detik (5000 ms).
 *   - Relay Nonaktif (Lampu Padam) selama 3 detik (3000 ms).
 *   - Berjalan secara siklus kontinu (looping) menggunakan non-blocking timer (millis()).
 *
 * Penjelasan Soal 3.b (Logika Pemicuan Relay - Active LOW vs Active HIGH):
 *   1. Modul Relay Active LOW:
 *      - Relay aktif saat pin kendali (IN) berlogika LOW (0V).
 *      - Rangkaian optocoupler internal terhubung ke VCC, sehingga logika LOW dari
 *        GPIO ESP32 menarik arus (sink current) ke ground, menyalakan transistor,
 *        menginduksi koil elektromagnetik, dan menutup sakelar COM - NO.
 *      - Saat pin IN berlogika HIGH (3.3V), optocoupler mati dan sakelar terbuka.
 *   2. Modul Relay Active HIGH:
 *      - Relay aktif saat pin kendali (IN) berlogika HIGH (3.3V).
 *      - GPIO mengalirkan arus (source current) untuk memicu transistor penggerak koil.
 *   3. Penentuan dalam Kode ini:
 *      - Pada simulator Wokwi (wokwi-relay-module), relay secara bawaan merespon
 *        logika HIGH untuk menghubungkan terminal COM ke NO (Active HIGH).
 *      - Untuk fleksibilitas implementasi hardware riil, digunakan konstanta:
 *        `RELAY_ON` dan `RELAY_OFF`. Jika menggunakan modul fisik tipe Active LOW,
 *        cukup ubah nilai `IS_ACTIVE_LOW` menjadi true.
 * ==============================================================================
 */

// Definisi Pin Relay
const int PIN_RELAY = 19;

// -----------------------------------------------------------------------------
// KONFIGURASI LOGIKA RELAY (Jawaban Soal 3.b)
// -----------------------------------------------------------------------------
// Ubah ke true jika menggunakan modul relay fisik yang bersifat Active LOW.
// Nilai false digunakan untuk simulator Wokwi atau modul Active HIGH.
const bool IS_ACTIVE_LOW = false; 

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
                IS_ACTIVE_LOW ? "Active LOW (LOW = ON, HIGH = OFF)" : "Active HIGH (HIGH = ON, LOW = OFF)");

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
