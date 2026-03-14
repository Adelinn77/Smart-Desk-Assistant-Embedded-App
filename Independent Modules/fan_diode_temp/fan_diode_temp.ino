#include <Wire.h>
#include <Adafruit_BMP280.h>

#define FAN_PIN 11   // pinul care merge prin rezistor la baza tranzistorului

Adafruit_BMP280 bmp;

const float TEMP_ON  = 20.0;
const float TEMP_OFF = 19.8;

bool fanOn = false;
unsigned long lastRead = 0;

void setup() {
  Serial.begin(9600);
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);

  Wire.begin(); // I2C

  // BMP280 poate fi 0x76 sau 0x77
  if (!bmp.begin(0x76) && !bmp.begin(0x77)) {
    Serial.println("❌ BMP280 NOT FOUND");
    while (1);
  }

  Serial.println("✅ BMP280 OK");
}

void loop() {
  if (millis() - lastRead >= 1000) {
    lastRead = millis();

    float temp = bmp.readTemperature();

    if (!fanOn && temp > TEMP_ON) {
      fanOn = true;
      digitalWrite(FAN_PIN, HIGH);
    } 
    else if (fanOn && temp < TEMP_OFF) {
      fanOn = false;
      digitalWrite(FAN_PIN, LOW);
    }

    Serial.print("Temp = ");
    Serial.print(temp, 2);
    Serial.print(" °C | Fan = ");
    Serial.println(fanOn ? "ON" : "OFF");
  }
}
