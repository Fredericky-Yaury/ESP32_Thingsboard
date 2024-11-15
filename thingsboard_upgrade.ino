// Library untuk DHT11, WiFi, dan Thingsboard
#include "DHT.h"
#include <WiFi.h>
#include <ThingsBoard.h>
#include <Arduino_MQTT_Client.h>

// Library untuk LCD
#include <LiquidCrystal_I2C.h>

// Define untuk DHT, LCD, dan RGB light
#define dhtPin 13
#define dhtType DHT11
DHT dht(dhtPin, dhtType);

int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

#define PIN_RED    4
#define PIN_GREEN  16
#define PIN_BLUE   17

// Untuk konektivitas WiFi dan Thingsboard
#define WIFI_AP "M3 - 619"
#define WIFI_PASS "fred619653"

#define TB_SERVER "thingsboard.cloud"
#define TOKEN "vJ0pGNhmdp8VIkL2GM2D"

constexpr uint16_t MAX_MESSAGE_SIZE = 128U;

WiFiClient espClient;
Arduino_MQTT_Client mqttClient(espClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);

// Function untuk koneksi ke WiFi
void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  int attempts = 0;
  
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    WiFi.begin(WIFI_AP, WIFI_PASS, 6);
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect to WiFi.");
  } else {
    Serial.println("\nConnected to WiFi");
  }
}

// Function untuk koneksi ke Thingsboard
void connectToThingsBoard() {
  if (!tb.connected()) {
    Serial.println("Connecting to ThingsBoard server");
    
    if (!tb.connect(TB_SERVER, TOKEN)) {
      Serial.println("Failed to connect to ThingsBoard");
    } else {
      Serial.println("Connected to ThingsBoard");
    }
  }
}

// Mengirim data ke Thingsboard dalam bentuk Json
void sendDataToThingsBoard(float temp, int hum, String tempStat, String humStat) {
  String jsonData = "{\"temperature\":" + String(temp) + ", \"humidity\":" + String(hum) + 
                    ", \"tempStatus\":" + String(tempStat) + ", \"humStatus\":" + String(humStat) + "}";
  tb.sendTelemetryJson(jsonData.c_str());
  Serial.println("Data sent");
}

// Hanya untuk mengubah warna RGB Light menjadi:
// - merah (keduanya tidak normal),
// - jingga (kelembapan tidak normal), 
// - kuning (suhu tidak normal), 
// - hijau (keduanya normal)

void changeLight(int gvalue){
  analogWrite(PIN_RED,   255);
  analogWrite(PIN_GREEN, gvalue);
  analogWrite(PIN_BLUE,  0);
}

// Setup components
void setup() {
  Serial.begin(9600);
  Serial.println("Greenhouse Program");
  
  connectToWiFi();
  connectToThingsBoard();

  dht.begin();

  lcd.init();                      
  lcd.backlight();

  pinMode(PIN_RED,   OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE,  OUTPUT);
}

void loop() {
  // Mulai koneksi ke WiFi
  connectToWiFi();

  // Baca DHT11
  float hum = dht.readHumidity();
  String humStat = "Normal";
  float temp = dht.readTemperature();
  String tempStat = "Normal";

  if (isnan(hum) || isnan(temp)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Tentukan status suhu dan kelembapan, ubah juga RGB Light nya
  if (hum > 78) humStat = "High";
  else if (hum < 65) humStat = "Low";
  else humStat = "Normal";

  if (temp > 28) tempStat = "High";
  else if (temp < 25) tempStat = "Low";
  else tempStat = "Normal";

  if (tempStat != "Normal" && humStat != "Normal") changeLight(0);
  else if (tempStat != "Normal") changeLight(30);
  else if (humStat != "Normal") changeLight(100);
  else changeLight(255);

  // Tampilkan di LCD
  lcd.setCursor(0, 0);
  lcd.print("Humd: ");
  lcd.print(hum);
  lcd.print(F("%"));

  lcd.setCursor(0,1);
  lcd.print("Temp: ");
  lcd.print(temp);
  lcd.print(" C");

  // Mulai koneksi ke Thingsboard
  if (!tb.connected()) {
    connectToThingsBoard();
  }

  // Kirim data ke Thingsboard
  sendDataToThingsBoard(temp, hum, tempStat, humStat);

  delay(3000);
  lcd.clear();

  // Lakukan loop untuk terus mengecek suhu dan kelembapan
  tb.loop();
}