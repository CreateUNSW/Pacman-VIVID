/*
Pacman Maze LED code
Library used: Adafruit NeoPixel: https://github.com/adafruit/Adafruit_NeoPixel
by Yunzhen Zhang

NOTE: Considering Uno's RAM is 2k and each NeoPixel requires 3 bytes of RAM regardless if they're on or off,
our maze will have 675 NeoPixels, which the Uno may not be able to support. I'm gonna test this with a Mega
when I have the chance to get my hands on one.

*/

#include <Adafruit_NeoPixel.h>
#include <avr/power.h>

#define TOTALLEDS 75 // that's 2.5m of led strips
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
  Serial.begin(9600);
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
  int x, y;
  x = 0;
  y = 1;
  //implement something here to get x/y    
  if ((x != -1)&&(y != -1)) {
    /*
    Serial.print("X: ");
    Serial.print(x);
    Serial.print("\tY: ");
    Serial.print(y);
    Serial.println();
   */
    mapxy[y][x] = 0;
    dots[x].setPixelColor(y, dots[x].Color(0,0,0));
    dots[x].show();    
  }
}

//shows the initital 9 x 9 grid
void showMap() {
  for (int i = 0; i < TOTALSTRIPS; i++) {
    for (int j = 0; j < TOTALLEDS; j++) {
      if (j % SPACING == 0) {  
        dots[i].setPixelColor(j, dots[i].Color(50,50,50));
      }
      dots[i].show();
    }
  }  
}
