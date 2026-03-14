#define PIR_PIN     2
#define BUZZER_PIN  12

const unsigned long WARMUP_MS     = 60000UL;
const unsigned long INACTIVITY_MS = 10000UL; // 10 sec test

const unsigned long BEEP_ON_MS  = 200;
const unsigned long BEEP_OFF_MS = 800;

unsigned long lastMotionTime = 0;
bool reminderActive = false;

bool beepState = false;
unsigned long nextBeepToggle = 0;

void setup() {
  Serial.begin(9600);
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.println("PIR warm-up 60s...");
  delay(WARMUP_MS);
  Serial.println("READY");

  lastMotionTime = millis();
}

void loop() {
  unsigned long now = millis();

  if (digitalRead(PIR_PIN) == HIGH) {
    lastMotionTime = now;
    if (reminderActive) {
      reminderActive = false;
      stopBuzzer();
      Serial.println("Motion detected. Reminder OFF.");
    }
  }

  if (!reminderActive && (now - lastMotionTime >= INACTIVITY_MS)) {
    reminderActive = true;
    Serial.println("Time to sit up and do some sport!");
    beepState = false;
    nextBeepToggle = now;
  }

  if (reminderActive) {
    runBuzzer(now);
  }
}

void runBuzzer(unsigned long now) {
  if (now < nextBeepToggle) return;

  if (!beepState) {
    digitalWrite(BUZZER_PIN, HIGH);
    beepState = true;
    nextBeepToggle = now + BEEP_ON_MS;
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    beepState = false;
    nextBeepToggle = now + BEEP_OFF_MS;
  }
}

void stopBuzzer() {
  digitalWrite(BUZZER_PIN, LOW);
  beepState = false;
}
