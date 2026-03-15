// ====== BLYNK AUTHENTICATION ======
#define BLYNK_TEMPLATE_ID "TMPL3-jlimjZ6"
#define BLYNK_TEMPLATE_NAME "Smart IOT System"
#define BLYNK_AUTH_TOKEN "8HlGgAduPphQoqZ3bRbT-ithqLkQThvt"

// ====== LIBRARIES ======
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ====== WIFI CREDENTIALS ======
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Raj";       
char pass[] = "Jai@Kisan01";

// ====== PIN CONFIGURATION ======
#define DHTPIN D4
#define DHTTYPE DHT11
#define SOIL A0
#define RELAY D0
#define RELAY_ON  LOW
#define RELAY_OFF HIGH
#define TRIG D5
#define ECHO D6
#define MANUAL_SWITCH D7

// ====== OBJECTS ======
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ====== CONTROL VARIABLES ======
bool autoMode = false;   // Auto (true) / Manual (false)
int pumpState = RELAY_OFF;

// ====== THRESHOLDS ======
int soilThreshold = 50;         // Soil % threshold
int waterTankHeight = 30;       // Tank height in cm

// ====== BLYNK FUNCTIONS ======
BLYNK_WRITE(V1) {  // Pump manual control
  if (!autoMode) {
    int pinValue = param.asInt();
    pumpState = (pinValue == 1) ? RELAY_ON : RELAY_OFF;
    digitalWrite(RELAY, pumpState);
    Blynk.virtualWrite(V7, (pumpState == RELAY_ON) ? 1 : 0);
  }
}

BLYNK_WRITE(V6) {  // Mode switch Auto/Manual
  int mode = param.asInt();
  autoMode = (mode == 1);  // 1 = Auto, 0 = Manual
}

void setup() {
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, RELAY_OFF);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  pinMode(MANUAL_SWITCH, INPUT_PULLUP);

  dht.begin();
  Blynk.begin(auth, ssid, pass);

  Wire.begin(D2, D1);  // LCD I2C pins
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("AgroSmart Ready");
  delay(2000);
  lcd.clear();
}

void loop() {
  Blynk.run();

  // ====== SENSOR READINGS ======
  int soilRaw = analogRead(SOIL);
  int soilPercent = map(soilRaw, 1023, 0, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);

  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();

  // Ultrasonic water level
  long duration, distance;
  digitalWrite(TRIG, LOW); delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  duration = pulseIn(ECHO, HIGH, 30000);
  distance = (duration * 0.034) / 2;

  int waterLevel = map(distance, waterTankHeight, 0, 0, 100);
  waterLevel = constrain(waterLevel, 0, 100);

  // ====== PHYSICAL SWITCH CONTROL ======
  if (digitalRead(MANUAL_SWITCH) == LOW && !autoMode) {
    pumpState = (pumpState == RELAY_ON) ? RELAY_OFF : RELAY_ON; // toggle
    digitalWrite(RELAY, pumpState);
    Blynk.virtualWrite(V1, (pumpState == RELAY_ON) ? 1 : 0);
    Blynk.virtualWrite(V7, (pumpState == RELAY_ON) ? 1 : 0);
    delay(500); // debounce
  }

  // ====== AUTO MODE LOGIC ======
  if (autoMode) {
    if (soilPercent < soilThreshold && waterLevel > 30) {  
      pumpState = RELAY_ON;   // Soil DRY + Tank not LOW → Pump ON
    } else {
      pumpState = RELAY_OFF;  // Soil WET OR Tank LOW → Pump OFF
    }
    digitalWrite(RELAY, pumpState);
    Blynk.virtualWrite(V1, (pumpState == RELAY_ON) ? 1 : 0);
    Blynk.virtualWrite(V7, (pumpState == RELAY_ON) ? 1 : 0);
  }

  // ====== STATUS STRINGS ======
  String soilStatus = (soilPercent < soilThreshold) ? "DRY" : "WET";

  String waterStatus;
  if (waterLevel > 70)      waterStatus = "HIGH";
  else if (waterLevel > 30) waterStatus = "MED";
  else                      waterStatus = "LOW";

  String pumpStatus = (pumpState == RELAY_ON) ? "ON" : "OFF";

  // ====== SEND TO BLYNK ======
  Blynk.virtualWrite(V2, soilPercent);   // Soil %
  Blynk.virtualWrite(V8, soilStatus);    // Soil DRY/WET
  Blynk.virtualWrite(V3, temp);          // Temp
  Blynk.virtualWrite(V4, (int)hum);      // Humidity (no decimal)
  Blynk.virtualWrite(V5, waterLevel);    // Tank %
  Blynk.virtualWrite(V9, waterStatus);   // Water HIGH/MED/LOW
  Blynk.virtualWrite(V7, (pumpState == RELAY_ON) ? 1 : 0);

  // ====== LCD DISPLAY ======
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("S:");
  lcd.print(soilStatus);
  lcd.print("");
  lcd.print(soilPercent);
  lcd.print("% T:");
  lcd.print(String(temp, 1)); // temperature with 1 decimal
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("H:");
  lcd.print((int)hum); // humidity as integer
  lcd.print("% W:");
  lcd.print(waterStatus);
  lcd.print(" P:");
  lcd.print(pumpStatus);

  delay(1000);
}
