# DOKUMENTASI & PANDUAN LENGKAP PENYELESAIAN UTS EMBEDDED SYSTEM
**Platform:** Wokwi Simulator | **Mikrokontroler:** ESP32 DevKit V1 | **Bahasa:** C++ / Arduino Core

---

## 📁 Struktur Direktori Folder `uts/`

Seluruh kode program dan skematik Wokwi telah disusun secara rapi per modul soal, serta dilengkapi dengan proyek terpadu (*All-in-One*):

```text
uts/
├── PANDUAN_UTS.md                <- Dokumen panduan lengkap ini
├── soal1_led_pwm/
│   ├── soal1_led_pwm.ino         <- Firmware Kontrol PWM LED 4-Level + Debouncing
│   └── diagram.json              <- Skematik Wokwi Soal 1
├── soal2_ldr_adc/
│   ├── soal2_ldr_adc.ino         <- Firmware Pembacaan ADC LDR 12-bit & % Kegelapan
│   └── diagram.json              <- Skematik Wokwi Soal 2
├── soal3_relay/
│   ├── soal3_relay.ino           <- Firmware Relay Siklus 5s ON / 3s OFF + Analisis Active LOW/HIGH
│   └── diagram.json              <- Skematik Wokwi Soal 3
├── soal4_servo_ultrasonic/
│   ├── soal4_servo_ultrasonic.ino<- Firmware Palang Otomatis Servo SG90 + HC-SR04
│   └── diagram.json              <- Skematik Wokwi Soal 4
├── soal5_mqtt_iot/
│   ├── soal5_mqtt_iot.ino        <- Firmware Telemetri & Kontrol Jarak Jauh MQTT
│   └── diagram.json              <- Skematik Wokwi Soal 5
└── proyek_terpadu_uts/
    ├── proyek_terpadu_uts.ino    <- Firmware Master (Integrasi 5 Soal Simultan Non-blocking)
    └── diagram.json              <- Skematik Wokwi Lengkap Seluruh Komponen
```

---

## 1. SOAL 1: Kontrol Kecerahan LED Bertingkat (PWM & GPIO)

### 1.1 Tabel Pengkabelan (Wiring)
| Komponen | Pin Komponen | Pin ESP32 | Keterangan |
| :--- | :--- | :--- | :--- |
| **LED** | Anoda (+) | `GPIO 16` | Dihubungkan seri dengan Resistor 220 $\Omega$ |
| | Katoda (-) | `GND` | Ground common |
| **Push Button** | Terminal 1 (1.l) | `GPIO 4` | Dikonfigurasi mode `INPUT_PULLUP` |
| | Terminal 2 (2.l) | `GND` | Ground |

### 1.2 Logika Kerja & Siklus Kecerahan
- **Resolusi PWM:** 8-bit ($0 - 255$) pada frekuensi $5\text{ kHz}$.
- **Tingkat Kecerahan:**
  1. **Padam (0%):** Duty Cycle = $0$
  2. **Redup (30%):** Duty Cycle = $\text{round}(255 \times 0.30) = 76$
  3. **Sedang (70%):** Duty Cycle = $\text{round}(255 \times 0.70) = 178$
  4. **Terang Maksimal (100%):** Duty Cycle = $255$
  5. Penekanan berikutnya kembali ke tingkat **Padam (0%)** secara sirkular.
- **Penanganan Debouncing:**
  Menggunakan timer non-blocking `millis()` dengan jeda $50\text{ ms}$ (`DEBOUNCE_DELAY`). Hal ini mencegah *false triggering* / lonjakan kontak mekanis saat tombol ditekan.

---

## 2. SOAL 2: Pembacaan Sensor Analog LDR dan Pengolahan Data ADC

### 2.1 Rangkaian Pembagi Tegangan (Voltage Divider)
- **Komponen:** 1x Sensor LDR (Photoresistor) dan 1x Resistor $10\text{ k}\Omega$.
- **Jalur Pengkabelan:**
  - $3.3\text{V} \rightarrow \text{LDR Pin 1}$
  - $\text{LDR Pin 2} \rightarrow \text{Node Tengah}$ (Terhubung ke `GPIO 34` & Resistor $10\text{ k}\Omega$ Pin 1)
  - $\text{Resistor } 10\text{ k}\Omega \text{ Pin 2} \rightarrow \text{GND}$

### 2.2 Pengolahan Data ADC 12-Bit
ESP32 memiliki ADC internal beresolusi 12-bit ($2^{12} = 4096$ tingkat kuantisasi, rentang $0 - 4095$):
- **Saat Kondisi Terang:** Resistansi LDR sangat kecil ($\sim 500\ \Omega$), sehingga tegangan jatuh pada resistor $10\text{ k}\Omega$ mendekati $3.3\text{ V}$. Nilai ADC mentah mendekati **$4095$**.
- **Saat Kondisi Gelap:** Resistansi LDR meningkat tajam ($\sim \text{ratusan k}\Omega$), tegangan pada node tengah ditarik ke ground mendekati $0\text{ V}$. Nilai ADC mentah mendekati **$0$**.

### 2.3 Formula Konversi Persentase Intensitas Kegelapan (0% - 100%)
$$\text{Persentase Kegelapan (\%)} = \frac{4095 - \text{rawADC}}{4095.0} \times 100\%$$
- Jika $\text{rawADC} = 4095 \implies \text{Kegelapan} = 0.0\%$ (Kondisi Sangat Terang)
- Jika $\text{rawADC} = 0 \implies \text{Kegelapan} = 100.0\%$ (Kondisi Gelap Gulita)
- Data ditampilkan ke Serial Monitor setiap $1$ detik secara non-blocking.

---

## 3. SOAL 3: Pengendalian Sakelar Beban Daya Menggunakan Modul Relay

### 3.1 Skema Pengkabelan
| Komponen | Pin Komponen | Pin ESP32 | Keterangan |
| :--- | :--- | :--- | :--- |
| **Modul Relay** | VCC | `3V3` atau `5V` | Catu daya modul relay |
| | GND | `GND` | Ground |
| | IN | `GPIO 19` | Sinyal kendali pemicu |
| **Terminal Beban**| COM (Common) | Sumber Daya ($3.3\text{V}$ / Jala-jala) | Terminal induk sakelar |
| | NO (Normally Open)| Anoda Lampu Beban | Terhubung saat relay aktif |

### 3.2 Siklus Waktu Operasional
- **Relay Aktif (Lampu Menyala):** Durasi $5\text{ detik}$ ($5000\text{ ms}$).
- **Relay Nonaktif (Lampu Padam):** Durasi $3\text{ detik}$ ($3000\text{ ms}$).
- Berulang secara kontinu (*looping*) tanpa fungsi `delay()` yang memblokir.

### 3.3 Penjelasan Logika Pemicuan Relay (Jawaban Soal 3.b)
1. **Active LOW (Karakteristik Umum Board Relay Fisik):**
   - Rangkaian input optocoupler pada modul terhubung ke rail VCC.
   - Ketika GPIO ESP32 mengeluarkan logika `LOW` ($0\text{ V}$), terjadi perbedaan potensial yang menyebabkan arus mengalir dari VCC menuju pin GPIO (*current sinking*). Optocoupler menyala, mentrigger transistor driver koil, sehingga koil elektromagnetik menarik kontak sakelar dari **NC** ke **NO** (Lampu ON).
   - Sebaliknya, logika `HIGH` ($3.3\text{ V}$) mematikan arus optocoupler (Lampu OFF).
2. **Active HIGH (Karakteristik Bawaan Simulator Wokwi):**
   - Transistor driver koil langsung dipicu ketika pin GPIO memberikan tegangan `HIGH` ($3.3\text{ V}$, *current sourcing*).
3. **Penerapan dalam Kode Program:**
   - Disediakan parameter flag: `const bool IS_ACTIVE_LOW = false;`
   - Logika penulisan pin otomatis disesuaikan:
     - `RELAY_ON  = IS_ACTIVE_LOW ? LOW  : HIGH;`
     - `RELAY_OFF = IS_ACTIVE_LOW ? HIGH : LOW;`

---

## 4. SOAL 4: Sistem Mekanisme Palang Otomatis (Servo SG90 & HC-SR04)

### 4.1 Tabel Pengkabelan
| Periferal | Pin Komponen | Pin ESP32 | Keterangan |
| :--- | :--- | :--- | :--- |
| **HC-SR04** | VCC | `5V` | Catu daya sensor ultrasonik |
| | Trig | `GPIO 5` | Pemicu pulsa ultrasonik |
| | Echo | `GPIO 17` | Penerima pantulan pulsa |
| | GND | `GND` | Ground |
| **Servo SG90** | Signal (PWM) | `GPIO 18` | Sinyal posisi servo ($50\text{ Hz}$) |
| | V+ | `5V` (atau `3V3`) | Catu daya motor servo |
| | GND | `GND` | Ground |

### 4.2 Algoritma Pengukuran Jarak & Penggerak Palang
1. Pulsa trigger dikirimkan selama $10\ \mu\text{s}$.
2. Durasi pantulan pulsa echo dihitung menggunakan `pulseIn(PIN_ECHO, HIGH, 30000)`.
3. Jarak dihitung dengan konstanta rambat suara:
   $$\text{Jarak (cm)} = \frac{\text{Durasi } (\mu\text{s}) \times 0.0343}{2}$$
4. **Kondisi Palang:**
   - $\text{Jarak} < 10.0\text{ cm} \implies$ Kendaraan terdeteksi $\rightarrow$ Poros Servo berputar ke sudut **$90^\circ$** (Palang Terbuka).
   - $\text{Jarak} \ge 10.0\text{ cm} \implies$ Area aman / kendaraan lewat $\rightarrow$ Poros Servo kembali ke sudut **$0^\circ$** (Palang Tertutup).

---

## 5. SOAL 5: Telemetri Data & Kontrol Jarak Jauh via Protokol MQTT

### 5.1 Parameter Jaringan & Konfigurasi Broker
- **Jaringan Wi-Fi:** `Wokwi-GUEST` (Tanpa kata sandi).
- **Public MQTT Broker:** `broker.emqx.io` (Port: `1883`).
- **NIM Placeholder:** Ganti konstanta `NIM = "YOUR_NIM";` dengan NIM Anda.
- **Topik Publish:** `unisda/informatika/[NIM]/cahaya`
- **Topik Subscribe:** `unisda/informatika/[NIM]/lampu`

### 5.2 Mekanisme Komunikasi
1. **Publish Telemetri (Tiap 5 Detik):**
   ESP32 membaca sensor LDR (GPIO 34), mengonversinya ke persentase kegelapan, dan mengirimkannya dalam format JSON:
   ```json
   {
     "nim": "rifqi",
     "uptime_sec": 25,
     "raw_adc": 1940,
     "kegelapan_pct": 52.63,
     "status_cahaya": "Redup"
   }
   ```
2. **Subscribe & Remote Control Lampu:**
   ESP32 mendengarkan pesan teks pada topik `unisda/informatika/[NIM]/lampu`:
   - Menerima string `"ON"` $\rightarrow$ Relay aktif, lampu menyala.
   - Menerima string `"OFF"` $\rightarrow$ Relay nonaktif, lampu padam.

---

## 🚀 Panduan Menjalankan Simulasi di Wokwi

1. Kunjungi [Wokwi Simulator](https://wokwi.com) dan pilih board **ESP32 (Arduino Core)**.
2. Pada tab **Library Manager** (`libraries.txt`), tambahkan pustaka berikut:
   ```text
   ESP32Servo
   PubSubClient
   ArduinoJson
   ```
3. Buka tab **`diagram.json`** di Wokwi, salin dan tempel isi dari file `diagram.json` pada modul soal yang ingin diuji.
4. Buka tab **sketch utama (`.ino`)**, salin dan tempel kode program `.ino` yang sesuai.
5. Tekan tombol **Start Simulation** (ikon Play) untuk menguji respon periferal dan memantau log pada Serial Monitor.
