#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include <ArduinoJson.h>

// Definisi Pin Sensor
const int PIN_DHT = 15;
const int PIN_TRIG = 5;
const int PIN_ECHO = 18;
const int PIN_GAS_ANALOG = 34;
const int PIN_LED_STATUS = 2; // LED Indikator Status / Gateway Heartbeat

// Inisialisasi Objek Sensor
DHTesp dhtSensor;

// Konfigurasi Jaringan Virtual Wokwi & MQTT Broker
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* mqtt_topic = "unisda/iot/pertemuan7/gateway/rifqi";

WiFiClient espClient;
PubSubClient client(espClient);

// Konfigurasi Siklus Gateway IoT
const unsigned long SAMPLING_INTERVAL = 1000;     // Baca sensor lokal setiap 1 detik
const unsigned long GATEWAY_PUSH_INTERVAL = 5000;   // Agregasi & kirim ke cloud tiap 5 detik
const float THRESHOLD_TANK_OVERFLOW = 10.0;       // Ambang batas kritis jarak tangki penuh (< 10 cm)
unsigned long lastSampleTime = 0;
unsigned long lastPushTime = 0;

// Variabel Penampung Edge Processing (Agregasi Data)
float sumTemperature = 0.0;
float sumHumidity = 0.0;
float sumDistance = 0.0;
float sumGasLevel = 0.0;
int sampleCount = 0;

// Fungsi Koneksi Ulang MQTT
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT Broker (EMQX)... ");
    String clientId = "ESP32Gateway-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("TERHUBUNG!");
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" Mencoba lagi dalam 2 detik...");
      delay(2000);
    }
  }
}

// Fungsi Pembacaan Sensor Jarak Ultrasonik (HC-SR04)
float readUltrasonicDistanceCM() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  
  long duration = pulseIn(PIN_ECHO, HIGH, 30000); // Timeout 30ms
  if (duration == 0) return -1.0; // Deteksi out-of-range
  return duration * 0.034 / 2;
}

// Fungsi Pembacaan Potensiometer sebagai Simulasi Sensor Gas (0 - 100%)
float readSimulatedGasLevel() {
  int rawADC = analogRead(PIN_GAS_ANALOG);
  return (rawADC / 4095.0) * 100.0;
}

// Fungsi Serialisasi JSON & Transmisi Data Uplink ke Cloud MQTT
void sendGatewayUplink(String systemStatus, float temp, float hum, float dist, float gas, int samples) {
  StaticJsonDocument<300> jsonDoc;
  jsonDoc["gateway_id"] = "GW-INFORMATIKA-07";
  jsonDoc["uptime_sec"] = millis() / 1000;
  jsonDoc["status"] = systemStatus;
  jsonDoc["samples_aggregated"] = samples;

  JsonObject telemetry = jsonDoc.createNestedObject("telemetry_avg");
  telemetry["temperature_c"] = serialized(String(temp, 2));
  telemetry["humidity_pct"] = serialized(String(hum, 2));
  telemetry["tank_distance_cm"] = serialized(String(dist, 2));
  telemetry["gas_level_pct"] = serialized(String(gas, 2));

  String jsonOutput;
  serializeJsonPretty(jsonDoc, jsonOutput);

  Serial.println("\n--------------------------------------------------");
  if (systemStatus == "TANK_OVERFLOW") {
    Serial.println(">>> [EDGE ALARM: TANK OVERFLOW!] MENGIRIM SEGERA KE CLOUD <<<");
  } else if (systemStatus == "ALERT_CRITICAL") {
    Serial.println(">>> [EDGE ALARM: ALERT CRITICAL] MENGIRIM SEGERA KE CLOUD <<<");
  } else {
    Serial.println(">>> [GATEWAY UPLINK] MENGIRIM PAKET KE CLOUD <<<");
  }
  Serial.println(jsonOutput);
  Serial.println("--------------------------------------------------\n");

  // Publish payload JSON ke MQTT Broker
  bool success = client.publish(mqtt_topic, jsonOutput.c_str());
  if (success) {
    Serial.printf(">>> [MQTT PUBLISH BERHASIL] Data terkirim ke topik: %s\n", mqtt_topic);
  } else {
    Serial.println(">>> [MQTT PUBLISH GAGAL] Ukuran payload melebihi buffer atau client terputus!");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n=============================================");
  Serial.println(">>> INISIALISASI IOT GATEWAY PERTEMUAN 7 <<<");
  Serial.println("=============================================");

  // Inisialisasi Pinout
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  dhtSensor.setup(PIN_DHT, DHTesp::DHT22);
  pinMode(PIN_LED_STATUS, OUTPUT);

  // Koneksi ke Jaringan Gateway Wi-Fi
  Serial.print("Menghubungkan ke Wi-Fi Gateway: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\n[GATEWAY ONLINE] IP Address Gateway: " + WiFi.localIP().toString());
  Serial.println("Gateway siap melakukan agregasi dan transmisi data telemetri.\n");
  digitalWrite(PIN_LED_STATUS, HIGH); // Indikator Gateway aktif dan online

  // Konfigurasi MQTT Server
  client.setServer(mqtt_server, mqtt_port);
  client.setBufferSize(512); // PENTING: Memperbesar buffer MQTT dari default 128 byte menjadi 512 byte agar muat menampung payload JSON agregasi
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  unsigned long currentMillis = millis();

  // TAHAP 1: DATA COLLECTION (Pengumpulan Data Lokal setiap 1 detik)
  if (currentMillis - lastSampleTime >= SAMPLING_INTERVAL) {
    lastSampleTime = currentMillis;

    TempAndHumidity dhtData = dhtSensor.getTempAndHumidity();
    float currentDistance = readUltrasonicDistanceCM();
    float currentGas = readSimulatedGasLevel();

    // Data Filtering: Validasi keabsahan data sebelum ditampung
    if (!isnan(dhtData.temperature) && !isnan(dhtData.humidity) && currentDistance >= 0) {
      sumTemperature += dhtData.temperature;
      sumHumidity += dhtData.humidity;
      sumDistance += currentDistance;
      sumGasLevel += currentGas;
      sampleCount++;
      
      Serial.printf("[EDGE SAMPLING #%d] Suhu: %.1f C | Hum: %.1f %% | Jarak: %.1f cm | Gas: %.1f %%\n",
                    sampleCount, dhtData.temperature, dhtData.humidity, currentDistance, currentGas);
      
      // Heartbeat blink menandakan aktivitas pembacaan sensor
      digitalWrite(PIN_LED_STATUS, LOW);
      delay(50);
      digitalWrite(PIN_LED_STATUS, HIGH);

      // =========================================================================
      // LOGIKA ALARM TEPI (EDGE THRESHOLD) - TANK_OVERFLOW
      // Jika jarak sensor HC-SR04 terdeteksi < 10 cm, status JSON seketika diubah
      // menjadi "TANK_OVERFLOW" dan langsung dipublish ke MQTT tanpa menunggu
      // siklus periodik 5 detik selesai!
      // =========================================================================
      if (currentDistance < THRESHOLD_TANK_OVERFLOW) {
        Serial.printf("\n⚠️ >>> [PERINGATAN DINI / ALARM TEPI] Jarak Terdeteksi: %.2f cm (< %.1f cm)! <<<\n",
                      currentDistance, THRESHOLD_TANK_OVERFLOW);
        Serial.println("⚠️ >>> Indikasi Tangki Penuh/Meluap! Mengirim status 'TANK_OVERFLOW' secara instan...");

        // Hitung rata-rata data hingga sampel kritis ini terdeteksi
        float avgTemp = sumTemperature / sampleCount;
        float avgHum  = sumHumidity / sampleCount;
        float avgDist = sumDistance / sampleCount;
        float avgGas  = sumGasLevel / sampleCount;

        // Transmisi segera ke Cloud MQTT
        sendGatewayUplink("TANK_OVERFLOW", avgTemp, avgHum, avgDist, avgGas, sampleCount);

        // Reset buffer agregasi dan perbarui lastPushTime agar siklus 5 detik dihitung ulang dari sekarang
        sumTemperature = 0.0;
        sumHumidity = 0.0;
        sumDistance = 0.0;
        sumGasLevel = 0.0;
        sampleCount = 0;
        lastPushTime = currentMillis;
      }
    }
  }

  // TAHAP 2 & 3: EDGE PROCESSING & GATEWAY PUSH (Setiap 5 detik untuk kondisi reguler)
  if (currentMillis - lastPushTime >= GATEWAY_PUSH_INTERVAL) {
    lastPushTime = currentMillis;

    if (sampleCount > 0) {
      // Pemrosesan Tepi: Hitung nilai rata-rata dari buffer lokal
      float avgTemp = sumTemperature / sampleCount;
      float avgHum  = sumHumidity / sampleCount;
      float avgDist = sumDistance / sampleCount;
      float avgGas  = sumGasLevel / sampleCount;

      // Evaluasi Kondisi Kritis Lokal (Edge Alarm / Filter)
      String systemStatus = "NORMAL";
      if (avgDist < THRESHOLD_TANK_OVERFLOW) {
        systemStatus = "TANK_OVERFLOW";
      } else if (avgTemp > 45.0 || avgGas > 75.0) {
        systemStatus = "ALERT_CRITICAL";
      }

      // TAHAP 4: PROTOCOL TRANSLATION, SERIALISASI JSON & MQTT PUBLISH REGULER
      sendGatewayUplink(systemStatus, avgTemp, avgHum, avgDist, avgGas, sampleCount);

      // Reset buffer agregasi untuk siklus berikutnya
      sumTemperature = 0.0;
      sumHumidity = 0.0;
      sumDistance = 0.0;
      sumGasLevel = 0.0;
      sampleCount = 0;
    }
  }
}
