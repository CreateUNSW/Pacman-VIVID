/*
Pacman Maze LED code
Library used: Adafruit NeoPixel: https://github.com/adafruit/Adafruit_NeoPixel
by Yunzhen Zhang

NOTE: Using an Arduino Mega for this, as Uno does not have enough SRAM to support all the NeoPixels
(Consider each neopixel takes 3 bytes of memory, we are using 600+ of them regardless whether they're on or off)

To do: investigate a bug when I include Serial.begin() in the code and the code refuses to compile...

*/

#include <Adafruit_NeoPixel.h>
#include <avr/power.h>

#define TOTALLEDS 66 // that's 2.5m of led strips
#define SPACING 8
#define TOTALSTRIPS 9 //for a 9x9 map

Adafruit_NeoPixel dots[9] =
{
Adafruit_NeoPixel(TOTALLEDS, 2, NEO_GRB + NEO_KHZ800),
Adafruit_NeoPixel(TOTALLEDS, 3, NEO_GRB + NEO_KHZ800),
Adafruit_NeoPixel(TOTALLEDS, 4, NEO_GRB + NEO_KHZ800),
Adafruit_NeoPixel(TOTALLEDS, 5, NEO_GRB + NEO_KHZ800),
Adafruit_NeoPixel(TOTALLEDS, 6, NEO_GRB + NEO_KHZ800),
Adafruit_NeoPixel(TOTALLEDS, 7, NEO_GRB + NEO_KHZ800),
Adafruit_NeoPixel(TOTALLEDS, 8, NEO_GRB + NEO_KHZ800),
Adafruit_NeoPixel(TOTALLEDS, 9, NEO_GRB + NEO_KHZ800),
Adafruit_NeoPixel(TOTALLEDS, 10, NEO_GRB + NEO_KHZ800)
};

byte mapxy[9][9] =
{{1, 1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1, 1, 1}};

void setup() {
  //Serial.begin(9600);
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
  /*
  int x, y;
  x = 8;
  y = 1;
  //implement something here to get x/y    
  if ((x != -1)&&(y != -1)) {
    
    Serial.print("X: ");
    Serial.print(x);
    Serial.print("\tY: ");
    Serial.print(y);
    Serial.println();
   
    mapxy[y][x] = 0;
    dots[x].setPixelColor(y*SPACING, dots[x].Color(0,0,0));
    dots[x].show();    
  }
  */
}

//shows the initital 9 x 9 grid
void showMap() {
  for (int i = 0; i < TOTALSTRIPS; i++) {
    for (int j = 0; j < TOTALLEDS; j++) {
      if (j % SPACING == 0) {  
        dots[i].setPixelColor(j, dots[i].Color(255,255,255));
      }
      dots[i].show();
    }
  }  
}
