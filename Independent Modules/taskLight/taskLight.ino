#include <Adafruit_NeoPixel.h>

#define LDR_PIN     A1
#define LED_PIN     6
#define NUM_LEDS    4   // ACUM 4 LED-URI

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Praguri (calibreaza dupa Serial Monitor)

const int THRESH_DARK_ON  = 800; 
const int THRESH_DARK_OFF = 750; 

const unsigned long LDR_INTERVAL_MS   = 200;
const unsigned long PRINT_INTERVAL_MS = 500;

unsigned long tLdr = 0;
unsigned long tPrint = 0;

int  ldrValue = 0;
bool ledsOn   = false;

void setup() {
  Serial.begin(9600);

  strip.begin();
  strip.clear();
  strip.setBrightness(25); // SAFE pentru alimentare din Arduino
  strip.show();

  Serial.println("LDR + WS2812 (4 LED-uri) - start");
}

void loop() {
  unsigned long now = millis();

  // Citire LDR
  if (now - tLdr >= LDR_INTERVAL_MS) {
    tLdr = now;
    ldrValue = analogRead(LDR_PIN);

    // Histerezis pe sensul corect:
    // aprinde cand devine intuneric (valoare mare)
    if (!ledsOn && ldrValue > THRESH_DARK_ON) {
      ledsOn = true;
      setStripOn();
    }
    // stinge cand revine lumina (valoare mai mica)
    else if (ledsOn && ldrValue < THRESH_DARK_OFF) {
      ledsOn = false;
      setStripOff();
    }
  }

  // Debug pe Serial
  if (now - tPrint >= PRINT_INTERVAL_MS) {
    tPrint = now;
    Serial.print("LDR=");
    Serial.print(ldrValue);
    Serial.print(" | LEDs=");
    Serial.println(ledsOn ? "ON" : "OFF");
  }
}

// Actiuni pe banda
void setStripOn() {
  // warm white slab (nu alb full)
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(255, 160, 80));
  }
  strip.show();
}

void setStripOff() {
  strip.clear();
  strip.show();
}
