const int PIR_PIN = 2;

int last = -1;

void setup() {
  Serial.begin(9600);
  pinMode(PIR_PIN, INPUT);
  Serial.println("Warming up PIR (~60s)...");
  delay(60000);              // warm-up
  Serial.println("READY");
}

void loop() {
  Serial.println(digitalRead(PIR_PIN));
  delay(200);
}
