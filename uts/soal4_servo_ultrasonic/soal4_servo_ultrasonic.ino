/**
 * ==============================================================================
 * UTS PRAKTIKUM EMBEDDED SYSTEM - SOAL 4
 * ==============================================================================
 * Judul     : Sistem Mekanisme Palang Otomatis Berbasis Motor Servo & HC-SR04
 * Komponen  : ESP32 DevKit, 1x Motor Servo SG90, 1x Sensor Ultrasonik HC-SR04
 * Pinout    : 
 *   - Servo Signal (PWM) -> GPIO 18
 *   - Servo VCC          -> 5V / Vin (atau 3V3)
 *   - Servo GND          -> GND
 *   - HC-SR04 Trig       -> GPIO 5
 *   - HC-SR04 Echo       -> GPIO 17
 *   - HC-SR04 VCC        -> 5V (atau 3V3 pada modul 3.3V kompatibel)
 *   - HC-SR04 GND        -> GND
 *
 * Logika Pengendalian Palang:
 *   a. Mengukur jarak rintangan di depan sensor menggunakan HC-SR04.
 *   b. Jarak < 10.0 cm  : Kendaraan/objek mendekat -> Servo bergerak ke sudut 90° (Palang Terbuka).
 *   c. Jarak >= 10.0 cm : Area bebas / objek menjauh -> Servo kembali ke sudut 0° (Palang Tertutup).
 *   d. Menggunakan library ESP32Servo yang optimal untuk arsitektur ESP32.
 * ==============================================================================
 */

#include <ESP32Servo.h>

// Definisi Pin Periferal
const int PIN_SERVO = 18;
const int PIN_TRIG  = 5;
const int PIN_ECHO  = 17;

// Batas Ambang Deteksi Objek (Threshold)
const float AMBANG_JARAK_BUKA = 10.0; // Batas jarak dalam centimeter (cm)

// Objek Motor Servo
Servo barrierServo;

// Variabel Status Palang
int currentAngle = -1; // -1 menandakan belum diinisialisasi
unsigned long lastMeasureTime = 0;
const unsigned long INTERVAL_UKUR = 200; // Pengukuran dilakukan setiap 200 ms untuk respon cepat

// Fungsi Mengukur Jarak dengan Sensor HC-SR04
float readDistanceCM() {
  // Pastikan pin Trig LOW selama 2 mikrodetik untuk sinyal bersih
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);

  // Kirim pulsa ultrasonik HIGH selama 10 mikrodetik
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // Baca durasi pantulan pulsa HIGH pada pin Echo (Timeout 30.000 µs = ±5 meter)
  long duration = pulseIn(PIN_ECHO, HIGH, 30000);

  // Jika timeout (tidak ada pantulan/luar jangkauan)
  if (duration == 0) {
    return 999.0; // Representasi jarak aman / tidak ada objek
  }

  // Rumus kecepatan suara: s = (v * t) / 2 dengan v = 0.0343 cm/µs
  float distance = (duration * 0.0343) / 2.0;
  return distance;
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n========================================================");
  Serial.println("UTS EMBEDDED SYSTEM - SOAL 4: PALANG OTOMATIS SERVO + HC-SR04");
  Serial.println("========================================================");

  // Inisialisasi Pin Ultrasonik
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  // Inisialisasi Motor Servo SG90 pada GPIO 18
  // Standard pulse width SG90: 500 µs (0°) hingga 2400 µs (180°) pada 50 Hz
  ESP32PWM::allocateTimer(0);
  barrierServo.setPeriodHertz(50);
  barrierServo.attach(PIN_SERVO, 500, 2400);

  // Posisi awal: Palang Tertutup (0°)
  barrierServo.write(0);
  currentAngle = 0;

  Serial.println("[Init] Sensor HC-SR04 aktif & Servo disetel ke 0° (Palang Tertutup).");
  Serial.printf("[Aturan] Jarak < %.1f cm -> Terbuka (90°) | Jarak >= %.1f cm -> Tertutup (0°)\n\n",
                AMBANG_JARAK_BUKA, AMBANG_JARAK_BUKA);
}

void loop() {
  unsigned long currentMillis = millis();

  // Pengukuran berkala setiap 200 ms (Non-blocking)
  if (currentMillis - lastMeasureTime >= INTERVAL_UKUR) {
    lastMeasureTime = currentMillis;

    float distance = readDistanceCM();

    // Evaluasi Logika Palang Otomatis
    if (distance < AMBANG_JARAK_BUKA) {
      // KONDISI OBJEK MENDEKAT (< 10 cm) -> BUKA PALANG (90°)
      if (currentAngle != 90) {
        barrierServo.write(90);
        currentAngle = 90;
        Serial.printf("[AKSI: PALANG TERBUKA] Objek Terdeteksi! Jarak: %.2f cm (< %.1f cm) -> Servo: 90°\n",
                      distance, AMBANG_JARAK_BUKA);
      }
    } else {
      // KONDISI AREA BEBAS / OBJEK MENJAUH (>= 10 cm) -> TUTUP PALANG (0°)
      if (currentAngle != 0) {
        barrierServo.write(0);
        currentAngle = 0;
        if (distance >= 900.0) {
          Serial.println("[AKSI: PALANG TERTUTUP] Area Bersih (Di luar jangkauan) -> Servo: 0°");
        } else {
          Serial.printf("[AKSI: PALANG TERTUTUP] Objek Menjauh! Jarak: %.2f cm (>= %.1f cm) -> Servo: 0°\n",
                        distance, AMBANG_JARAK_BUKA);
        }
      }
    }
  }
}
