#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

// PINI 
#define LDR_PIN         A1
#define PIR_PIN         2

#define LAMP_PIN        6
#define STATUS_PIN      5

#define BUZZER_PIN      12
#define FAN_PIN         11

#define LAMP_LEDS       4
#define STATUS_LEDS     1

Adafruit_NeoPixel lamp(LAMP_LEDS, LAMP_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel statusLed(STATUS_LEDS, STATUS_PIN, NEO_GRB + NEO_KHZ800);

Adafruit_BMP280 bmp;       // I2C
bool bmpOk = false;

const unsigned long INACTIVITY_MS = 5000UL; 

//buzzer
const unsigned long BEEP_ON_MS  = 200;
const unsigned long BEEP_OFF_MS = 800;
bool buzzerOn = false;
unsigned long nextBeepToggle = 0;

//timers 
const unsigned long LIGHT_INTERVAL_MS  = 200;
const unsigned long MOTION_INTERVAL_MS = 50;
const unsigned long ANIM_INTERVAL_MS   = 20;
const unsigned long PRINT_INTERVAL_MS  = 500;

// TEMP/FAN
const float TEMP_THRESHOLD = 15.0f;
const unsigned long TEMP_INTERVAL_MS = 1000;

// Fan cycle: 10s ON, 5s OFF, repeat 3 times, then STOP
const unsigned long FAN_ON_MS  = 10000UL;
const unsigned long FAN_OFF_MS = 5000UL;
const int FAN_MAX_CYCLES = 3;

enum FanState { FAN_IDLE, FAN_ON, FAN_OFF, FAN_LOCKED };
FanState fanState = FAN_IDLE;
unsigned long fanStateStart = 0;
int fanCyclesDone = 0;
float tempC = -1000.0f;

unsigned long tLight=0, tMotion=0, tAnim=0, tPrint=0, tTemp=0;

//stare
int  ldrValue = 0;
bool lampOn = false;

bool pirState = false;
unsigned long lastMotionTime = 0;
bool inactive = false;

bool animActive = false;
unsigned long animStart = 0;

bool reminderActive = false;

void setup() {
  Serial.begin(9600);

  pinMode(PIR_PIN, INPUT);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);

  lamp.begin();
  lamp.clear();
  lamp.setBrightness(25);
  lamp.show();

  statusLed.begin();
  statusLed.clear();
  statusLed.setBrightness(40);
  statusLed.show();

  bmpOk = initBMP280_robust();
  if (bmpOk) Serial.println("BMP280 OK");
  else       Serial.println("BMP280 NOT FOUND (robust init failed)");

  lastMotionTime = millis();
  Serial.println("Start: LDR + PIR + buzzer + BMP280 + fan cycles");

  animActive = true;
  animStart = millis();
  reminderActive = false;

  statusLed.setPixelColor(0, statusLed.Color(0, 0, 40)); 
  statusLed.show();

}

void loop() {
  unsigned long now = millis();

  if (now - tLight >= LIGHT_INTERVAL_MS) {
    tLight = now;
    taskLight();
  }

  if (now - tMotion >= MOTION_INTERVAL_MS) {
    tMotion = now;
    taskMotion(now);
    taskInactivity(now);
  }

  if (now - tAnim >= ANIM_INTERVAL_MS) {
    tAnim = now;
    taskStatusLed(now); 
    taskReminderBuzzer(now);

  }

  if (bmpOk && (now - tTemp >= TEMP_INTERVAL_MS)) {
    tTemp = now;
    taskTemperature();
  }

  taskFan(now);

  if (now - tPrint >= PRINT_INTERVAL_MS) {
    tPrint = now;
    printStatus(now);
  }
}

void taskReminderBuzzer(unsigned long now) {
  if (!reminderActive) return;

  if (now < nextBeepToggle) return;

  if (!buzzerOn) {
    buzzerOn = true;
    digitalWrite(BUZZER_PIN, HIGH);
    nextBeepToggle = now + BEEP_ON_MS;
  } else {
    buzzerOn = false;
    digitalWrite(BUZZER_PIN, LOW);
    nextBeepToggle = now + BEEP_OFF_MS;
  }
}


void taskStatusLed(unsigned long now) {
  unsigned long t = now % 1000UL;
  int level = (t < 500) ? (t / 4) : ((1000 - t) / 4);
  if (level < 0) level = 0;
  if (level > 125) level = 125;

  if (reminderActive) {
    // inactive -> red pulse
    statusLed.setPixelColor(0, statusLed.Color(level, 0, 0));
  } else {
    // active -> blue pulse
    statusLed.setPixelColor(0, statusLed.Color(0, 0, level));
  }
  statusLed.show();
}


bool initBMP280_robust() {
  Wire.begin();
  Wire.setClock(100000);     // 100kHz 
  delay(200);                // 

  for (int attempt = 1; attempt <= 5; attempt++) {
    Serial.print("BMP init attempt ");
    Serial.println(attempt);

    if (bmp.begin(0x76)) {
      bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                      Adafruit_BMP280::SAMPLING_X2,
                      Adafruit_BMP280::SAMPLING_X16,
                      Adafruit_BMP280::FILTER_X16,
                      Adafruit_BMP280::STANDBY_MS_500);
      Serial.println("BMP found @0x76");
      return true;
    }

    delay(200);
  }
  return false;
}

//LAMPA (banda 4 LED-uri)
void taskLight() {
  ldrValue = analogRead(LDR_PIN);

  if (!lampOn && ldrValue >= 940) {
    lampOn = true;
    lampOnRender();
  } 
  else if (lampOn && ldrValue <= 930) {
    lampOn = false;
    lampOffRender();
  }
}

void lampOnRender() {
  for (int i = 0; i < LAMP_LEDS; i++) {
    lamp.setPixelColor(i, lamp.Color(255, 160, 80));
  }
  lamp.show();
}

void lampOffRender() {
  lamp.clear();
  lamp.show();
}

// PIR 
void taskMotion(unsigned long now) {
  bool v = (digitalRead(PIR_PIN) == HIGH);

  if (v && !pirState) {
    lastMotionTime = now;

    if (reminderActive) {
      reminderActive = false;
      stopBuzzer();
      Serial.println("Motion detected. Reminder OFF.");
    } else {
      Serial.println("MOTION!");
    }
  }

  pirState = v;
}

void taskInactivity(unsigned long now) {
  bool newInactive = (now - lastMotionTime >= INACTIVITY_MS);

  if (newInactive != inactive) {
    inactive = newInactive;

    if (inactive) {
      reminderActive = true;
      Serial.println("Time to take a break and move");

      buzzerOn = false;
      nextBeepToggle = now;
    } else {
      reminderActive = false;
      stopBuzzer();
      Serial.println("Active again");
    }
  }
}

void taskStatusAnimation(unsigned long now) {
  if (!animActive) return;

  unsigned long t = (now - animStart) % 1000UL;
  int level = (t < 500) ? (t / 4) : ((1000 - t) / 4);
  if (level < 0) level = 0;
  if (level > 125) level = 125;

  statusLed.setPixelColor(0, statusLed.Color(0, 0, level));
  statusLed.show();
}

void taskReminder(unsigned long now) {
  if (!reminderActive) return;

  unsigned long t = now % 1000UL;
  int level = (t < 500) ? (t / 4) : ((1000 - t) / 4);
  if (level > 125) level = 125;
  statusLed.setPixelColor(0, statusLed.Color(level, 0, 0));
  statusLed.show();

  if (now < nextBeepToggle) return;

  if (!buzzerOn) {
    buzzerOn = true;
    digitalWrite(BUZZER_PIN, HIGH);
    nextBeepToggle = now + BEEP_ON_MS;
  } else {
    buzzerOn = false;
    digitalWrite(BUZZER_PIN, LOW);
    nextBeepToggle = now + BEEP_OFF_MS;
  }
}

void stopBuzzer() {
  buzzerOn = false;
  digitalWrite(BUZZER_PIN, LOW);
}

//TEMP 
void taskTemperature() {
  tempC = bmp.readTemperature();
}

// FAN: 10s ON, 5s OFF, x3, apoi STOP
void taskFan(unsigned long now) {
  if (!bmpOk) return;

  if (fanState == FAN_LOCKED) {
    digitalWrite(FAN_PIN, LOW);
    return;
  }

  if (fanState == FAN_IDLE) {
    digitalWrite(FAN_PIN, LOW);

    if (tempC > TEMP_THRESHOLD) {
      fanState = FAN_ON;
      fanStateStart = now;
      digitalWrite(FAN_PIN, HIGH);
      Serial.println("Fan cycle START (10s ON / 5s OFF) x3");
    }
    return;
  }

  if (fanState == FAN_ON) {
    if (now - fanStateStart >= FAN_ON_MS) {
      fanState = FAN_OFF;
      fanStateStart = now;
      digitalWrite(FAN_PIN, LOW);
    }
    return;
  }

  if (fanState == FAN_OFF) {
    if (now - fanStateStart >= FAN_OFF_MS) {
      fanCyclesDone++;

      Serial.print("Fan cycle completed: ");
      Serial.print(fanCyclesDone);
      Serial.print("/");
      Serial.println(FAN_MAX_CYCLES);

      if (fanCyclesDone >= FAN_MAX_CYCLES) {
        fanState = FAN_LOCKED;
        digitalWrite(FAN_PIN, LOW);
        Serial.println("Fan STOPPED permanently (3 cycles done).");
      } else {
        fanState = FAN_ON;
        fanStateStart = now;
        digitalWrite(FAN_PIN, HIGH);
      }   
    }
    return;
  }
}

void printStatus(unsigned long now) {
  Serial.print("LDR=");
  Serial.print(ldrValue);
  Serial.print(" lamp=");
  Serial.print(lampOn ? "ON" : "OFF");

  Serial.print(" | PIR=");
  Serial.print(pirState ? 1 : 0);

  Serial.print(" inactive=");
  Serial.print(inactive ? "YES" : "NO");

  Serial.print(" reminder=");
  Serial.print(reminderActive ? "ON" : "OFF");

  Serial.print(" lastMotion=");
  Serial.print((now - lastMotionTime)/1000);
  Serial.print("s");

  Serial.print(" | T=");
  Serial.print(tempC, 1);
  Serial.print("C fanState=");
  Serial.print((int)fanState);
  Serial.print(" cycles=");
  Serial.print(fanCyclesDone);

  Serial.println();
}
