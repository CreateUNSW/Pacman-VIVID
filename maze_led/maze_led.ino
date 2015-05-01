#include <Adafruit_NeoPixel.h>
#include <avr/power.h>

#define PIN 6

#define TOTALLEDS 150
#define SPACING 8

Adafruit_NeoPixel dots = Adafruit_NeoPixel(TOTALLEDS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
   dots.begin(); 
}

void loop() {
  for (int i = 0; i < TOTALLEDS; i++) {
    if (i % SPACING == 0) {  
      dots.setPixelColor(i, dots.Color(50,50, 50));
      dots.show();
    }
    delay(50);
  }
}
