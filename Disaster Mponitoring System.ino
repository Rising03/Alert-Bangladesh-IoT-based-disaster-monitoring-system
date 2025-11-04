#include <WiFi.h>
#include <ThingSpeak.h>
#include <DHT.h>
#include <Adafruit_BMP280.h>
#include <SPI.h>

// ---------- Wi-Fi ----------
const char* ssid = "X";
const char* password = "12345678";

// ---------- ThingSpeak ----------
unsigned long myChannelNumber = 1234567;  // Replace with actual numeric Channel ID
const char* myWriteAPIKey = "5FOKR9WU2CNC3VWU";

WiFiClient client;

// ---------- Pin Definitions ----------
#define TRIG_PIN 25
#define ECHO_PIN 26
#define SOIL_PIN 33
#define RAIN_PIN 32
#define DHT_PIN 27
#define DHT_TYPE DHT22
#define LDR_PIN 35
#define BMP_SCK  18
#define BMP_MISO 19
#define BMP_MOSI 23
#define BMP_CS   5

// ---------- Sensor Setup ----------
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_BMP280 bmp280(BMP_CS, BMP_MOSI, BMP_MISO, BMP_SCK);

// ---------- Timing ----------
#define READ_INTERVAL 20000             // 20 seconds
#define RESET_INTERVAL 86400000UL       // 24 hours
unsigned long lastReadTime = 0;
unsigned long lastResetTime = 0;

// ---------- Rain Accumulation ----------
float totalRainMM = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== ESP32 Disaster Monitoring + ThingSpeak ===");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(SOIL_PIN, INPUT);
  pinMode(RAIN_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);

  dht.begin();

  if (!bmp280.begin()) {
    Serial.println("BMP280 not found!");
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\n✅ Wi-Fi connected.");
  ThingSpeak.begin(client);

  lastResetTime = millis();
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastResetTime >= RESET_INTERVAL) {
    totalRainMM = 0;
    lastResetTime = currentTime;
    Serial.println("🌧️ Rain accumulation reset (24h)");
  }

  if (currentTime - lastReadTime >= READ_INTERVAL) {
    lastReadTime = currentTime;
    Serial.println("----- Sensor Readings -----");

    // 1️⃣ Rain Sensor
    int rainValue = analogRead(RAIN_PIN);
    float rainPercent = map(rainValue, 0, 4095, 100, 0);
    float rainMM = (rainPercent / 100.0) * 0.05;
    totalRainMM += rainMM;
    int isRaining = (rainPercent > 20) ? 1 : 0;

    // 2️⃣ Soil Moisture
    int soilValue = analogRead(SOIL_PIN);
    float soilPercent = map(soilValue, 0, 4095, 100, 0);

    // 3️⃣ DHT22
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    // 4️⃣ Ultrasonic Sensor
    digitalWrite(TRIG_PIN, LOW); delayMicroseconds(5);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH, 60000);
    float distance_cm = (duration == 0) ? -1 : (duration * 0.0343) / 2;

    // 5️⃣ BMP280 Pressure
    float pressure = bmp280.readPressure() / 100.0F;

    // 6️⃣ LDR (Digital)
    int ldrState = digitalRead(LDR_PIN);  // 1 = bright, 0 = dark

    // 📡 Print to Serial
    Serial.printf("Rain: %.3f mm | Total: %.3f mm | Raining: %s\n", rainMM, totalRainMM, isRaining ? "Yes" : "No");
    Serial.printf("Soil: %.1f %%\n", soilPercent);
    Serial.printf("Temp: %.1f C | Hum: %.1f %%\n", temp, hum);
    Serial.printf("Water Level: %.1f cm\n", distance_cm);
    Serial.printf("Pressure: %.1f hPa\n", pressure);
    Serial.printf("Light: %s\n", ldrState ? "Bright" : "Dark");

    // 📤 Send to ThingSpeak
    ThingSpeak.setField(1, temp);
    ThingSpeak.setField(2, hum);
    ThingSpeak.setField(3, soilPercent);
    ThingSpeak.setField(4, isRaining);       // ✅ 1 = Yes, 0 = No
    ThingSpeak.setField(5, totalRainMM);
    ThingSpeak.setField(6, distance_cm);
    ThingSpeak.setField(7, pressure);
    ThingSpeak.setField(8, ldrState);

    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (x == 200) {
      Serial.println("✅ Data sent to ThingSpeak!");
    } else {
      Serial.print("❌ ThingSpeak error, code: ");
      Serial.println(x);
    }

    Serial.println("----------------------------\n");
  }
}
