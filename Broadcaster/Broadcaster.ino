#include <SPI.h>
#include "RF24.h"
/*
Pacman Broadcaster code
by Mark Johnson

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

/*
 * PACMAN RF Data Struct and utilities
 */

#include "RF24.h"

/*
 * Game Struct definitions
 */

typedef struct _position {
    uint8_t x : 4;
    uint8_t y : 4;
} position;

typedef char heading;

typedef struct _robot {
    position p;
    heading h;
} robot;

// Contains information about all robots currently in play.
typedef struct _game_state {
    uint8_t header;
    uint8_t command;
    uint8_t override_dir;	
    robot pac;
    robot g1;
    robot g2;
    robot g3;
} game_state;

/*
 * Game state definitions
 */ 
// Header
#define HEADER 0xC8

// Game commands
#define NOP      0
#define START    1
#define STOP     2
#define HIDE     3
#define PAUSE    4
#define RESUME   5 
#define RUN_MODE 6
#define MANUAL_OVERRIDE 7

#define GAME_SIZE sizeof(game_state)

/*
 * Radio definitions
 */
#define NUM_RETRANS 5
#define BROADCAST_CHANNEL 72


/*
 * Functions to be used by broadcaster
 */

void init_radio(void);
void init_game(void);
void broadcast_game(void);

/*
 * Global Variables
 */
// Only need a single pipe due to one-way comms.
RF24 radio(48,49);
const uint64_t pipe = 0xF0F0F0F0E1LL;
game_state game;
uint8_t __space[21];

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


void setup() {
  Serial.begin(57600);
  for (int i = 0; i < TOTALSTRIPS; i++) {
     dots[i].begin();
     //clears all leds
     dots[i].show(); 
  }
  //initialise map
  showMap();
  init_radio();
  init_game();
}

int i;
void loop() {
  if (Serial.available()) {    
    i = 0;
    delay(1);
    while(Serial.available() && i < GAME_SIZE) {
        ((uint8_t *)&game)[i++] = Serial.read();
        delayMicroseconds(250);
    }
    delay(1);
    for (i = 0; i < NUM_RETRANS; i++) 
      broadcast_game();
  }

}
// Game must remain stopped until matlab is ready
void init_game() {
  game.command = STOP;
}

void init_radio() {
  //Serial.print("init_radio...");
  radio.begin();
  radio.setRetries(15,15);
  radio.openWritingPipe(pipe);
  radio.setChannel(BROADCAST_CHANNEL);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.stopListening();
  radio.setAutoAck(false);
  radio.setPayloadSize(32);
  //Serial.println("done");
}

void print_game() {
  int i;
  for (i = 0; i < GAME_SIZE; i++) {
    Serial.print("Byte ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(((char *)&game)[i], DEC);
  }
  Serial.println();
   
}
// Used by Matlab connected arduino 
void broadcast_game() {
  set_checksum();
  radio.write((char *)&game, GAME_SIZE);
}

// A simple checksum calculator, operates within 1ms
void set_checksum(void) {
  uint8_t   i, j, sum = 0;
  for (i = 1; i < GAME_SIZE; i++) {
    for (j = 0; j < 8; j++) {
      sum += (((char *)&game)[i] & (1 << j)) >> j;
    }
  }
  game.header = sum;
}

/** MAZE LED FUNCTIONS **/

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
