/**
 * ==============================================================================
 * UTS PRAKTIKUM EMBEDDED SYSTEM - SOAL 1
 * ==============================================================================
 * Judul     : Kontrol Kecerahan LED Bertingkat Menggunakan PWM & GPIO
 * Komponen  : ESP32 DevKit, 1x LED, 1x Resistor 220 Ohm, 1x Push Button
 * Pinout    : 
 *   - LED Anoda        -> GPIO 16 (melalui Resistor 220 Ohm)
 *   - LED Katoda       -> GND
 *   - Push Button Pin 1-> GPIO 4 (Mode INPUT_PULLUP)
 *   - Push Button Pin 2-> GND
 *
 * Logika Kerja:
 *   - Tingkat kecerahan LED bertambah bertingkat setiap tombol ditekan:
 *     0: Padam (0%   - Duty: 0)
 *     1: Redup (30%  - Duty: 77)
 *     2: Sedang (70% - Duty: 179)
 *     3: Terang Maksimal (100% - Duty: 255)
 *     -> Kembali ke Padam pada penekanan berikutnya.
 *   - Dilengkapi penanganan software debouncing berbasis timer non-blocking (millis()).
 * ==============================================================================
 */

// Definisi Pin
const int PIN_LED = 16;
const int PIN_BUTTON = 4;

// Konfigurasi PWM ESP32
const int PWM_CHANNEL = 0;
const int PWM_FREQ = 5000;       // Frekuensi 5 kHz
const int PWM_RESOLUTION = 8;    // Resolusi 8-bit (Rentang nilai duty cycle: 0 - 255)

// Definisi Nilai Duty Cycle Kecerahan (8-bit: 0 - 255)
const int DUTY_LEVELS[4] = {
  0,                              // 0%   - Padam
  (int)(255 * 0.30),              // 30%  - Redup (76/77)
  (int)(255 * 0.70),              // 70%  - Sedang (178/179)
  255                             // 100% - Terang Maksimal
};

const char* LEVEL_NAMES[4] = {
  "Padam (0%)",
  "Redup (30%)",
  "Sedang (70%)",
  "Terang Maksimal (100%)"
};

// Variabel Status Sistem
int currentLevel = 0;            // Status tingkat kecerahan saat ini (0 - 3)

// Variabel Debouncing Tombol
int buttonState = HIGH;          // Nilai stabil tombol saat ini (INPUT_PULLUP: HIGH = dilepas, LOW = ditekan)
int lastRawButtonReading = HIGH; // Nilai pembacaan mentah tombol sebelumnya
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50; // Jeda debouncing 50 ms

// Fungsi untuk menerapkan duty cycle ke LED (Kompatibel dengan semua versi ESP32 Core)
void setLedBrightness(int duty) {
  #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(PIN_LED, duty);
  #else
    ledcWrite(PWM_CHANNEL, duty);
  #endif
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n========================================================");
  Serial.println("UTS EMBEDDED SYSTEM - SOAL 1: KONTROL PWM LED BERTINGKAT");
  Serial.println("========================================================");

  // Inisialisasi Tombol dengan internal Pull-Up
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  // Inisialisasi Kanal PWM LED
  #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(PIN_LED, PWM_FREQ, PWM_RESOLUTION);
  #else
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PIN_LED, PWM_CHANNEL);
  #endif

  // Set kondisi awal: Padam (0%)
  setLedBrightness(DUTY_LEVELS[currentLevel]);

  Serial.printf("[Sistem Siap] Tingkat Awal: %s (Duty Cycle: %d/255)\n", 
                LEVEL_NAMES[currentLevel], DUTY_LEVELS[currentLevel]);
  Serial.println("Tekan push button pada GPIO 4 untuk menaikkan tingkat kecerahan.\n");
}

void loop() {
  // 1. Baca status input tombol fisik
  int rawReading = digitalRead(PIN_BUTTON);

  // 2. Deteksi perubahan pembacaan mentah (fluktuasi/bouncing)
  if (rawReading != lastRawButtonReading) {
    lastDebounceTime = millis(); // Reset timer debouncing
  }

  // 3. Jika sinyal stabil lebih dari DEBOUNCE_DELAY, proses penekanan tombol
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    // Jika ada perubahan status stabil
    if (rawReading != buttonState) {
      buttonState = rawReading;

      // Terdeteksi transisi penekanan (HIGH ke LOW / Tombol ditekan ke GND)
      if (buttonState == LOW) {
        // Naikkan level kecerahan secara sirkular (0 -> 1 -> 2 -> 3 -> 0)
        currentLevel = (currentLevel + 1) % 4;

        // Terapkan nilai PWM ke LED
        setLedBrightness(DUTY_LEVELS[currentLevel]);

        // Tampilkan feedback ke Serial Monitor
        Serial.printf("[BUTTON CLICK] Berpindah ke Tingkat: %s | Nilai PWM: %d/255\n",
                      LEVEL_NAMES[currentLevel], DUTY_LEVELS[currentLevel]);
      }
    }
  }

  // Simpan nilai pembacaan terakhir
  lastRawButtonReading = rawReading;
}
