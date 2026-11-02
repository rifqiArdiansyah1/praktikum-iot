# Praktikum Embedded System - Sistem Monitoring Lingkungan IoT (ESP32)

Repositori ini berisi kode firmware, skema virtual Wokwi, dan dokumentasi analisis studi kasus untuk modul praktikum Embedded System berbasis ESP32.

## 📌 Ringkasan Proyek
Proyek ini mengimplementasikan sistem monitoring suhu dan kelembaban lingkungan secara real-time dengan sensor digital DHT22 dan ESP32 di simulator Wokwi. Firmware dirancang menggunakan pendekatan **non-blocking timer (`millis()`)** dan dilengkapi fitur **Multi-Condition Alert** serta evaluasi **Deep Sleep** untuk efisiensi daya.

## 🛠️ Komponen & Pinout Wiring
| Komponen Periferal | Pin Komponen | Pin ESP32 (DevKit V1) | Keterangan / Fungsi |
| :--- | :--- | :--- | :--- |
| **Sensor DHT22** | VCC | `3V3` | Catu daya 3.3V |
| | SDA / Data | `GPIO 15` | Jalur data serial satu kabel |
| | GND | `GND` | Ground common |
| **LED Hijau** | Anoda (+) | `GPIO 2` | Indikator kondisi normal (via R 220Ω) |
| | Katoda (-) | `GND` | Ground |
| **LED Merah** | Anoda (+) | `GPIO 4` | Indikator alarm kritis (via R 220Ω) |
| | Katoda (-) | `GND` | Ground |
| **Piezo Buzzer** | Positif (+) | `GPIO 5` | Peringatan audio 1 kHz |
| | Negatif (-) | `GND` | Ground |

## ⚙️ Logika Multi-Condition Alert
Sistem akan memicu alarm (LED Merah ON & Buzzer berbunyi) jika:
1. **Suhu Lingkungan > 38.0 °C**, **ATAU**
2. **Kelembaban Udara < 20.0 % RH**

Jika kedua parameter berada dalam batas aman:
- LED Hijau ON
- LED Merah OFF
- Buzzer nonaktif (Mati)
- Serial Monitor menampilkan status normal

## 📁 Struktur Berkas
- `embeded.ino` : Firmware utama ESP32 (Arduino framework/C++) dengan implementasi timer `millis()` dan logika multi-kondisi.
- `diagram.json` : Konfigurasi rangkaian virtual untuk platform Wokwi Simulator.
- `LAPORAN_PRAKTIKUM.md` : Dokumentasi pengujian, analisis manajemen daya (Deep Sleep & baterai Li-ion 2000 mAh), dan kesimpulan.

## 🚀 Cara Menjalankan di Wokwi
1. Buka [Wokwi Simulator](https://wokwi.com) dan buat project baru **ESP32 (Arduino framework)**.
2. Tambahkan library `DHT sensor library for ESPx` pada tab **Library Manager**.
3. Salin kode dari `embeded.ino` ke tab sketch utama.
4. Salin isi `diagram.json` ke tab `diagram.json`.
5. Klik tombol **Start Simulation** dan ubah slider parameter DHT22 untuk menguji respon sistem.
