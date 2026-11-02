# DRAFT LAPORAN & JAWABAN STUDI KASUS PRAKTIKUM EMBEDDED SYSTEM
**Topik:** Perancangan Firmware Non-Blocking & Sistem Monitoring Lingkungan IoT Menggunakan ESP32 di Wokwi Simulator  
**Subjek:** CPMK - Mahasiswa Mampu Mengonfigurasi I/O, Menulis Firmware Non-Blocking, Menguji Konektivitas Wi-Fi, dan Menganalisis Manajemen Daya (Deep Sleep)

---

## I. IDENTIFIKASI MODIFIKASI FIRMWARE (STUDI KASUS 1: MULTI-CONDITION ALERT)

### 1.1 Logika Pemicu Baru
Berdasarkan ketentuan Studi Kasus VII:
- **Ambang Suhu Kritis:** $> 38.0^\circ\text{C}$
- **Ambang Kelembaban Kritis:** $< 20.0\%\text{ RH}$
- **Hubungan Logika:** Operasi Logika `OR` (`||`), yaitu alarm aktif jika suhu melebihi batas **ATAU** kelembaban di bawah batas aman.
- **Respon Aktuator:**
  - **Kondisi Alarm (Salah satu atau kedua syarat terpenuhi):** LED Merah menyala (`HIGH`), LED Hijau padam (`LOW`), dan Piezo Buzzer berbunyi (`tone(BUZZER_PIN, 1000)`).
  - **Kondisi Normal (Kedua parameter dalam batas aman):** LED Merah padam (`LOW`), LED Hijau menyala (`HIGH`), dan Piezo Buzzer mati (`noTone(BUZZER_PIN)`).

### 1.2 Pembedaan Pesan Serial Monitor
Firmware dirancang memberikan diagnosis teks spesifik pada Serial Monitor:
1. **Kritis Suhu Saja:**  
   `>>> [ALARM KRITIS - SUHU] Suhu Melebihi Batas Aman! Nilai: XX.XX °C (Ambang: 38.0 °C) <<<`
2. **Kritis Kelembaban Saja:**  
   `>>> [ALARM KRITIS - KELEMBABAN] Kelembaban Udara Terlalu Kering! Nilai: XX.XX % (Ambang: 20.0 %) <<<`
3. **Kritis Ganda (Keduanya terlampaui):**  
   `>>> [ALARM KRITIS - GANDA] Suhu Terlalu Tinggi (XX.XX °C > 38.0 °C) DAN Kelembaban Terlalu Kering (XX.XX % < 20.0 %)! <<<`
4. **Kondisi Normal:**  
   `    [Status: NORMAL] Lingkungan Terkendali (Suhu & Kelembaban Aman).`

---

## II. ANALISIS STUDI KASUS 2: DEEP-SLEEP EVALUATION (MAKS. 1 HALAMAN)

### 2.1 Arsitektur Manajemen Daya ESP32
ESP32 memiliki beberapa mode daya operasional:
1. **Active Mode:** CPU bekerja penuh (80/160/240 MHz), Wi-Fi & Bluetooth aktif. Konsumsi arus: $80 - 240\text{ mA}$.
2. **Modem-sleep / Light-sleep:** CPU diturunkan frekuensinya atau dihentikan sementara, modul RF dimatikan. Konsumsi arus: $0.8 - 3\text{ mA}$.
3. **Deep-sleep Mode:** CPU Core, Wi-Fi, Bluetooth, dan sebagian besar periferal dimatikan total. Hanya domain daya **RTC (Real-Time Clock)**, memori RTC, dan RTC Timer yang tetap beroperasi. Konsumsi arus chip murni hanya berkisar **$10 - 15\ \mu\text{A}$**.

### 2.2 Skenario Pengujian: Siklus Transmisi 10 Menit & Baterai Li-ion 2000 mAh
- **Kapasitas Baterai ($C$):** $2000\text{ mAh}$ (Tegangan nominal $3.7\text{ V}$, teregulasi ke $3.3\text{ V}$).
- **Periode Siklus ($T_{siklus}$):** $10\text{ menit} = 600\text{ detik}$.
- **Fase Aktif ($T_{aktif}$):**
  - Proses: Booting, inisialisasi sensor DHT22, koneksi Wi-Fi, transmisi data/telemetri, dan masuk ke sleep.
  - Durasi: $T_{aktif} \approx 3\text{ detik}$.
  - Arus rata-rata saat aktif ($I_{aktif}$): $\approx 130\text{ mA}$.
- **Fase Tidur ($T_{sleep}$):**
  - Durasi: $T_{sleep} = 600 - 3 = 597\text{ detik}$.
  - Arus saat Deep Sleep ($I_{sleep}$):
    - Pada board hemat daya/custom board: $\approx 0.02\text{ mA}$ ($20\ \mu\text{A}$).
    - Pada ESP32 DevKit standar (terbebani LDO AMS1117 & IC CP2102/CH340): $\approx 3 - 6\text{ mA}$.

### 2.3 Perhitungan Matematis Konsumsi Arus & Estimasi Daya Tahan Baterai

#### A. Rata-rata Konsumsi Arus per Siklus ($I_{avg}$):
$$I_{avg} = \frac{(I_{aktif} \times T_{aktif}) + (I_{sleep} \times T_{sleep})}{T_{siklus}}$$

Substitusi nilai untuk desain hardware optimal ($I_{sleep} = 0.02\text{ mA}$):
$$I_{avg} = \frac{(130\text{ mA} \times 3\text{ s}) + (0.02\text{ mA} \times 597\text{ s})}{600\text{ s}}$$
$$I_{avg} = \frac{390 + 11.94}{600} = \frac{401.94}{600} \approx 0.67\text{ mA}$$

#### B. Estimasi Masa Pakai Baterai ($T_{life}$):
$$\text{Masa Pakai (Jam)} = \frac{\text{Kapasitas Baterai}}{\text{Arus Rata-rata}} = \frac{2000\text{ mAh}}{0.67\text{ mA}} \approx 2985\text{ Jam}$$
$$\text{Masa Pakai (Hari)} = \frac{2985}{24} \approx 124.3\text{ Hari}\ (\pm 4.1\text{ Bulan})$$

*Perbandingan Tanpa Deep Sleep:*  
Jika perangkat terus menyala pada mode aktif ($I \approx 130\text{ mA}$):  
$$\text{Masa Pakai} = \frac{2000\text{ mAh}}{130\text{ mA}} \approx 15.38\text{ Jam}\ (< 1\text{ Hari})$$
> **Kesimpulan Efisiensi:** Penerapan Deep Sleep dengan transmisi berkala 10 menit meningkatkan masa pakai baterai hingga **$\approx 194$ kali lebih lama**.

### 2.4 Implementasi Kode Deep Sleep ESP32
Dalam arsitektur Deep Sleep, loop `void loop()` tidak digunakan karena perangkat langsung dimatikan setelah proses transmisi selesai. Seluruh alur diletakkan di `setup()`:

```cpp
#include <WiFi.h>
#include "DHTesp.h"
#include "esp_sleep.h"

#define uS_TO_S_FACTOR 1000000ULL  // Konversi mikrodetik ke detik
#define TIME_TO_SLEEP  600         // Waktu tidur: 600 detik (10 menit)

void setup() {
  Serial.begin(115200);
  
  // 1. Baca Sensor DHT22 & Inisialisasi Wi-Fi
  // 2. Kirim Data Telemetri ke Broker MQTT / Server Cloud HTTP
  
  // 3. Konfigurasi Timer Wakeup ESP32
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("[Deep Sleep] ESP32 memasuki mode tidur selama 10 menit...");
  Serial.flush(); // Pastikan transmisi serial tuntas sebelum sleep
  
  // 4. Masuk ke Deep Sleep
  esp_deep_sleep_start();
}

void loop() {
  // Tidak pernah dieksekusi dalam mode Deep Sleep
}
```

---

## III. CONTOH LOG SERIAL MONITOR UNTUK LAPORAN PRAKTIKUM

### 1. Inisialisasi Node & Wi-Fi
```text
========================================================
  SISTEM MONITORING LINGKUNGAN BERBASIS IOT (ESP32)
  Modul Praktikum Embedded System - Mode Multi-Kondisi
========================================================
[Init] Sensor DHT22 terkonfigurasi pada GPIO 15.
[Wi-Fi] Menghubungkan ke SSID: Wokwi-GUEST
..........
[Wi-Fi Connected!] Berhasil terhubung ke jaringan.
[Wi-Fi Info] Alamat IP: 10.0.0.2
--------------------------------------------------------
Sistem Siap! Memulai loop pembacaan non-blocking...
```

### 2. Skenario 1: Kondisi Normal (Aman)
*(Pengaturan DHT22: Suhu 28.0 °C, Kelembaban 60.0 %)*
```text
[Telemetri] Suhu: 28.00 °C | Kelembaban: 60.00 % | RSSI: -65 dBm
    [Status: NORMAL] Lingkungan Terkendali (Suhu & Kelembaban Aman).
--------------------------------------------------------------------------------
```
*Status Aktuator:* LED Hijau ON, LED Merah OFF, Buzzer OFF.

### 3. Skenario 2: Kondisi Kritis Suhu
*(Pengaturan DHT22: Suhu 41.5 °C, Kelembaban 55.0 %)*
```text
[Telemetri] Suhu: 41.50 °C | Kelembaban: 55.00 % | RSSI: -65 dBm
>>> [ALARM KRITIS - SUHU] Suhu Melebihi Batas Aman! Nilai: 41.50 °C (Ambang: 38.0 °C) <<<
--------------------------------------------------------------------------------
```
*Status Aktuator:* LED Hijau OFF, LED Merah ON, Buzzer ON (1000 Hz).

### 4. Skenario 3: Kondisi Kritis Kelembaban
*(Pengaturan DHT22: Suhu 31.0 °C, Kelembaban 14.5 %)*
```text
[Telemetri] Suhu: 31.00 °C | Kelembaban: 14.50 % | RSSI: -65 dBm
>>> [ALARM KRITIS - KELEMBABAN] Kelembaban Udara Terlalu Kering! Nilai: 14.50 % (Ambang: 20.0 %) <<<
--------------------------------------------------------------------------------
```
*Status Aktuator:* LED Hijau OFF, LED Merah ON, Buzzer ON (1000 Hz).

### 5. Skenario 4: Kondisi Kritis Ganda
*(Pengaturan DHT22: Suhu 42.0 °C, Kelembaban 12.0 %)*
```text
[Telemetri] Suhu: 42.00 °C | Kelembaban: 12.00 % | RSSI: -65 dBm
>>> [ALARM KRITIS - GANDA] Suhu Terlalu Tinggi (42.00 °C > 38.0 °C) DAN Kelembaban Terlalu Kering (12.00 % < 20.0 %)! <<<
--------------------------------------------------------------------------------
```
*Status Aktuator:* LED Hijau OFF, LED Merah ON, Buzzer ON (1000 Hz).

---

## IV. KESIMPULAN PRAKTIKUM
1. **Penerapan Pola Non-Blocking (`millis()`):** Penggunaan fungsi `millis()` berhasil menggantikan fungsi `delay()`, sehingga siklus eksekusi CPU tidak terhenti (*unblocked*) dan mikrokontroler tetap responsif dalam membaca data sensor serta mengendalikan aktuator secara tepat waktu.
2. **Logika Deteksi Multi-Kondisi:** Implementasi ambang batas kritis gabungan ($T > 38^\circ\text{C}$ atau $RH < 20\%$) memberikan respon aktuasi lokal seketika (LED Merah & Buzzer 1 kHz) dengan pesan diagnosa yang jelas untuk setiap jenis pemicu.
3. **Optimasi Konsumsi Daya:** Analisis membuktikan bahwa sistem IoT bertenaga baterai (Li-ion 2000 mAh) wajib memanfaatkan fitur **ESP32 Deep Sleep**. Dengan transmisi telemetri berkala setiap 10 menit, masa pakai perangkat dapat melonjak dari sekitar 15 jam menjadi lebih dari 124 hari (~4 bulan).
