# DOKUMENTASI & PANDUAN LENGKAP PENYELESAIAN UTS EMBEDDED SYSTEM
**Platform:** Wokwi Simulator | **Mikrokontroler:** ESP32 DevKit V1 / C V4 | **Bahasa:** C++ / Arduino Core

---

## 📁 Struktur Direktori Folder `uts/`

Seluruh kode program dan skematik Wokwi telah diverifikasi dan disinkronkan secara presisi dengan pustaka komponen simulator Wokwi:

```text
uts/
├── PANDUAN_UTS.md                 <- Dokumen panduan lengkap ini
├── soal1_led_pwm/                 <- SOAL 1: Kontrol Kecerahan LED Bertingkat
│   ├── soal1_led_pwm.ino          <- Firmware Kontrol PWM LED 4-Level + Debouncing
│   └── diagram.json               <- Skematik Wokwi Soal 1
├── soal2_ldr_adc/                 <- SOAL 2: Pembacaan ADC Sensor LDR 12-Bit
│   ├── soal2_ldr_adc.ino          <- Firmware Pembacaan ADC LDR 12-bit & % Kegelapan
│   └── diagram.json               <- Skematik Wokwi Soal 2 (wokwi-photoresistor-sensor)
├── soal3_relay/                   <- SOAL 3: Sakelar Beban Relay 5s ON / 3s OFF
│   ├── soal3_relay.ino            <- Firmware Relay Siklus 5s ON / 3s OFF + Analisis Active LOW
│   └── diagram.json               <- Skematik Wokwi Soal 3
├── soal4_servo_ultrasonic/        <- SOAL 4: Palang Pintu Otomatis Servo SG90 + HC-SR04
│   ├── soal4_servo_ultrasonic.ino <- Firmware Palang Otomatis Servo SG90 + HC-SR04
│   └── diagram.json               <- Skematik Wokwi Soal 4
├── soal5_mqtt_iot/                <- SOAL 5: Telemetri & Remote Control Protokol MQTT
│   ├── soal5_mqtt_iot.ino         <- Firmware Telemetri & Kontrol Jarak Jauh MQTT
│   └── diagram.json               <- Skematik Wokwi Soal 5
└── proyek_terpadu_uts/            <- PROYEK MASTER: Integrasi 5 Soal Simultan
    ├── proyek_terpadu_uts.ino     <- Firmware Master (5 Subsistem Simultan Non-blocking)
    └── diagram.json               <- Skematik Wokwi Lengkap Seluruh Komponen
```

---

## 1. SOAL 1: Kontrol Kecerahan LED Bertingkat (PWM & GPIO)

### 1.1 Tabel Pengkabelan (Wiring)
| Komponen | Pin Komponen | Pin ESP32 | Keterangan |
| :--- | :--- | :--- | :--- |
| **LED** | Anoda (+) | `GPIO 16` | Dihubungkan seri dengan Resistor 220 $\Omega$ |
| | Katoda (-) | `GND` | Ground common |
| **Push Button** | Terminal 1 (`1.l`) | `GPIO 4` | Dikonfigurasi mode `INPUT_PULLUP` |
| | Terminal 2 (`2.l`) | `GND` | Ground |

### 1.2 Logika Kerja & Siklus Kecerahan
- **Resolusi PWM:** 8-bit ($0 - 255$) pada frekuensi $5\text{ kHz}$.
- **Tingkat Kecerahan:**
  1. **Padam (0%):** Duty Cycle = $0$
  2. **Redup (30%):** Duty Cycle = $\text{round}(255 \times 0.30) = 76$
  3. **Sedang (70%):** Duty Cycle = $\text{round}(255 \times 0.70) = 178$
  4. **Terang Maksimal (100%):** Duty Cycle = $255$
  5. Penekanan berikutnya kembali ke tingkat **Padam (0%)** secara sirkular.
- **Penanganan Debouncing:**
  Menggunakan timer non-blocking `millis()` dengan jeda $50\text{ ms}$ (`DEBOUNCE_DELAY`).

---

## 2. SOAL 2: Pembacaan Sensor Analog LDR dan Pengolahan Data ADC

### 2.1 Rangkaian Modul Sensor LDR di Wokwi
- **Komponen Virtual Wokwi:** `wokwi-photoresistor-sensor` (modul LDR interaktif dengan slider Lux).
- **Jalur Pengkabelan:**
  - `ldr1:VCC` $\rightarrow$ `3V3` ESP32
  - `ldr1:GND` $\rightarrow$ `GND` ESP32
  - `ldr1:AO` $\rightarrow$ `GPIO 34` ESP32 (ADC1_CH6 - Input Only)

### 2.2 Karakteristik ADC & Konversi Persentase Kegelapan
Pada simulator Wokwi (`wokwi-photoresistor-sensor`):
- **Kondisi Terang (High Lux):** Resistansi LDR sangat kecil $\implies$ Tegangan AO rendah ($\sim 0.04\text{ V} - 0.2\text{ V}$) $\implies$ Nilai ADC mentah mendekati **$0$**.
- **Kondisi Gelap (Low Lux):** Resistansi LDR sangat besar $\implies$ Tegangan AO tinggi ($\sim 3.3\text{ V}$) $\implies$ Nilai ADC mentah mendekati **$4095$**.

### 2.3 Formula Konversi Persentase Intensitas Kegelapan (0% - 100%)
$$\text{Persentase Kegelapan (\%)} = \frac{\text{rawADC}}{4095.0} \times 100\%$$
- Nilai $\text{rawADC} \approx 4095 \implies \text{Kegelapan} = 100.0\%$ (Kondisi Gelap Gulita)
- Nilai $\text{rawADC} \approx 0 \implies \text{Kegelapan} = 0.0\%$ (Kondisi Sangat Terang)
- Data ditampilkan ke Serial Monitor setiap $1$ detik secara non-blocking.

---

## 3. SOAL 3: Pengendalian Sakelar Beban Daya Menggunakan Modul Relay

### 3.1 Skema Pengkabelan
| Komponen | Pin Komponen | Pin ESP32 | Keterangan |
| :--- | :--- | :--- | :--- |
| **Modul Relay** | VCC | `5V` | Catu daya modul relay |
| | GND | `GND` | Ground |
| | IN | `GPIO 19` | Sinyal kendali pemicu |
| **Terminal Beban**| COM (Common) | `3V3` ESP32 | Terminal induk sakelar |
| | NO (Normally Open)| Anoda Lampu (via R 220 $\Omega$) | Terhubung saat relay aktif |

### 3.2 Siklus Waktu Operasional
- **Relay Aktif (Lampu Menyala):** Durasi $5\text{ detik}$ ($5000\text{ ms}$).
- **Relay Nonaktif (Lampu Padam):** Durasi $3\text{ detik}$ ($3000\text{ ms}$).
- Berulang secara kontinu (*looping*) tanpa fungsi `delay()` yang memblokir.

### 3.3 Penjelasan Logika Pemicuan Relay (Jawaban Soal 3.b)
1. **Active LOW (Bawaan Wokwi & Modul Fisik):**
   - Modul `wokwi-relay-module` secara default menggunakan transistor NPN yang menghubungkan terminal **COM ke NO saat pin IN berlogika LOW (0V)**.
   - Sinyal `LOW` mengalirkan arus *sink* ke GPIO ESP32, mengaktifkan koil relay sehingga lampu menyala.
   - Sinyal `HIGH` mematikan koil, menghubungkan COM ke NC sehingga lampu padam.
2. **Active HIGH:**
   - Relay aktif saat pin kendali diberi tegangan `HIGH` ($3.3\text{ V}$, *source current*).
3. **Penerapan dalam Kode:**
   - Parameter disetel: `const bool IS_ACTIVE_LOW = true;` agar lampu beban menyala dan padam tepat sesuai siklus simulasi Wokwi.

---

## 4. SOAL 4: Sistem Mekanisme Palang Otomatis (Servo SG90 & HC-SR04)

### 4.1 Tabel Pengkabelan
| Periferal | Pin Komponen | Pin ESP32 | Keterangan |
| :--- | :--- | :--- | :--- |
| **HC-SR04** | VCC | `5V` | Catu daya sensor ultrasonik |
| | Trig | `GPIO 5` | Pemicu pulsa ultrasonik ($10\ \mu\text{s}$) |
| | Echo | `GPIO 17` | Penerima pantulan pulsa |
| | GND | `GND` | Ground |
| **Servo SG90** | Signal (`PWM`) | `GPIO 18` | Sinyal kendali PWM |
| | V+ | `5V` | Catu daya motor servo |
| | GND | `GND` | Ground |

### 4.2 Logika Palang Otomatis
- Sensor mengukur jarak rintangan setiap $200\text{ ms}$.
- Jika $\text{Jarak} < 10.0\text{ cm} \implies$ Objek terdeteksi $\rightarrow$ Poros Servo berputar ke sudut **$90^\circ$** (Palang Terbuka).
- Jika $\text{Jarak} \ge 10.0\text{ cm} \implies$ Area aman / objek menjauh $\rightarrow$ Poros Servo kembali ke sudut **$0^\circ$** (Palang Tertutup).

---

## 5. SOAL 5: Telemetri Data & Kontrol Jarak Jauh via Protokol MQTT

### 5.1 Parameter Jaringan & Broker
- **Jaringan Wi-Fi:** `Wokwi-GUEST` (Tanpa kata sandi).
- **Public MQTT Broker:** `broker.emqx.io` (Port: `1883`).
- **NIM Placeholder:** Ganti konstanta `NIM = "rifqi";` dengan NIM Anda.
- **Topik Publish:** `unisda/informatika/[NIM]/cahaya`
- **Topik Subscribe:** `unisda/informatika/[NIM]/lampu`

### 5.2 Mekanisme Komunikasi
1. **Publish Telemetri (Tiap 5 Detik):**
   ESP32 membaca sensor LDR (GPIO 34), mengonversinya ke persentase kegelapan, dan mengirimkannya dalam format JSON:
   ```json
   {
     "nim": "rifqi",
     "uptime_sec": 15,
     "raw_adc": 3850,
     "kegelapan_pct": 94.02,
     "status_cahaya": "Gelap"
   }
   ```
2. **Subscribe & Remote Control Lampu:**
   ESP32 mendengarkan pesan teks pada topik `unisda/informatika/[NIM]/lampu`:
   - Menerima string `"ON"` $\rightarrow$ Relay aktif, lampu menyala.
   - Menerima string `"OFF"` $\rightarrow$ Relay nonaktif, lampu padam.

---

## 🚀 Panduan Menjalankan Simulasi di Wokwi

1. Buka [Wokwi Simulator](https://wokwi.com) dan pilih board **ESP32 (Arduino Core)**.
2. Pada tab **Library Manager** (`libraries.txt`), tambahkan pustaka berikut:
   ```text
   ESP32Servo
   PubSubClient
   ArduinoJson
   ```
3. Buka tab **`diagram.json`** di Wokwi, salin dan tempel isi dari file `diagram.json` pada modul soal yang ingin diuji.
4. Buka tab **sketch utama (`.ino`)**, salin dan tempel kode program `.ino` yang sesuai.
5. Tekan tombol **Start Simulation** (ikon Play) untuk menguji respon periferal dan memantau log pada Serial Monitor.
