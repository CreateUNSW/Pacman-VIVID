#include <Adafruit_NeoPixel.h>
#include <avr/power.h>

#define TOTALLEDS 75 // that's 2.5m of led strips
#define SPACING 8
#define TOTALSTRIPS 9 //for a 9x9 map

Adafruit_NeoPixel dots[9] =
{Adafruit_NeoPixel(TOTALLEDS, 2, NEO_GRB + NEO_KHZ800),
Adafruit_NeoPixel(TOTALLEDS, 3, NEO_GRB + NEO_KHZ800),
Adafruit_NeoPixel(TOTALLEDS, 4, NEO_GRB + NEO_KHZ800),
Adafruit_NeoPixel(TOTALLEDS, 5, NEO_GRB + NEO_KHZ800),
Adafruit_NeoPixel(TOTALLEDS, 6, NEO_GRB + NEO_KHZ800),
Adafruit_NeoPixel(TOTALLEDS, 7, NEO_GRB + NEO_KHZ800),
Adafruit_NeoPixel(TOTALLEDS, 8, NEO_GRB + NEO_KHZ800),
Adafruit_NeoPixel(TOTALLEDS, 9, NEO_GRB + NEO_KHZ800),
Adafruit_NeoPixel(TOTALLEDS, 10, NEO_GRB + NEO_KHZ800)
};

int mapxy[9][9] = {1};

void setup() {
   // initialise each strip, starting from D2
   for (int i = 0; i < TOTALSTRIPS; i++) {
     dots[i].begin();
     //clears all leds
     dots[i].show(); 
   }
   //initialise map
   showMap();
}

void loop() {
  //if given a coordinate
    
}

//shows the initital 9 x 9 grid
void showMap() {
  for (int i = 0; i < TOTALSTRIPS; i++) {
    for (int j = 0; i < TOTALLEDS; j++) {
      if (j % SPACING == 0) {  
        dots[i].setPixelColor(j, dots[i].Color(50,50,50));
        dots[i].show();
      }
      delay(20);
    }
  }  
}
