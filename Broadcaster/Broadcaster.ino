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
// Radio headers
#include <SPI.h>
#include "RF24.h"
// LED headers
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

// Game commands
#define NOP             B0000000
#define START           B0000001
#define STOP            B0000010
#define HIDE            B0000100
#define PAUSE           B0001000
#define RESUME          B0010000
#define MUSIC_COMMAND   B0100000
#define MANUAL_OVERRIDE B1000000

#define GAME_SIZE sizeof(game_state)

/*
 * Radio definitions
 */
#define NUM_RETRANS 5
#define BROADCAST_CHANNEL 72

/*
 * Audio definitions
 */
#define MUS_MASK     B1110000
#define MUS_BEGIN    B0010000
#define MUS_VICTORY  B0100000
#define MUS_DEATH    B1000000

/*
 * Functions to be used by broadcaster
 */

void init_radio(void);
void init_game(void);
void init_audio(void);
void audio_play( int sound );
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
{
  {1, 1, 1, 1, 0, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 0, 1, 1, 1, 1},
  {0, 1, 1, 1, 1, 1, 1, 1, 0},
  {1, 1, 1, 0, 0, 0, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 0, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1, 1}
};

byte startup[6][9][9] = {

{
  /***         C          ***/
  {0, 1, 1, 1, 1, 1, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 1, 1, 1, 1, 0, 0, 0}
},

{
  /***         R          ***/
  {0, 1, 1, 1, 1, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 1, 0, 0, 0},
  {0, 1, 0, 0, 0, 1, 0, 0, 0},
  {0, 1, 0, 0, 0, 1, 0, 0, 0},
  {0, 1, 1, 1, 1, 0, 0, 0, 0},
  {0, 1, 1, 0, 0, 0, 0, 0, 0},
  {0, 1, 0, 1, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 1, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 1, 0, 0, 0}
},

{
  /***         E          ***/
  {0, 1, 1, 1, 1, 1, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 1, 1, 1, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 1, 1, 1, 1, 0, 0, 0}
},

{
  /***         A          ***/
  {0, 0, 0, 1, 0, 0, 0, 0, 0},
  {0, 0, 1, 1, 1, 0, 0, 0, 0},
  {0, 0, 1, 0, 1, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 1, 0, 0, 0},
  {0, 1, 1, 1, 1, 1, 0, 0, 0},
  {0, 1, 0, 0, 0, 1, 0, 0, 0},
  {0, 1, 0, 0, 0, 1, 0, 0, 0},
  {0, 1, 0, 0, 0, 1, 0, 0, 0},
  {0, 1, 0, 0, 0, 1, 0, 0, 0}
},

{
  /***         T          ***/
  {0, 1, 1, 1, 1, 1, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0, 0}
},
{
  /***         E          ***/
  {0, 1, 1, 1, 1, 1, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 1, 1, 1, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 0, 0, 0, 0, 0, 0, 0},
  {0, 1, 1, 1, 1, 1, 0, 0, 0}
}
  
  
};

Adafruit_NeoPixel dots[9] =
{
  Adafruit_NeoPixel(TOTALLEDS,  2, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(TOTALLEDS,  3, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(TOTALLEDS,  4, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(TOTALLEDS,  5, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(TOTALLEDS,  6, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(TOTALLEDS,  7, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(TOTALLEDS,  8, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(TOTALLEDS,  9, NEO_GRB + NEO_KHZ800),
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
  blankMap();
  create_leds();
  showMap();
  init_radio();
  init_game();
  init_audio();
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
    // TURN OFF LIGHT WHERE PACMAN IS
    // Try and save time by only updating when a change has been made
    if (mapxy[game.pac.p.x][game.pac.p.y] == 1) {
      mapxy[game.pac.p.x][game.pac.p.y] = 0;
      audio_play(2); // "eat dot" sound
      showMap();
    }
    
    // Special matlab command for an audio change
    if ((game.command & MUSIC_COMMAND)== MUSIC_COMMAND) {
        //Serial.println("got music");
        switch(game.override_dir & MUS_MASK){
          case MUS_BEGIN:   audio_play( 1 ); break;
          case MUS_VICTORY: audio_play( 3 ); break;
          case MUS_DEATH:   audio_play( 4 ); break;
          default: break;
       }
       // Clear music information for clean broadcast to robots
       game.command &= ~MUSIC_COMMAND;
       game.override_dir &= ~MUS_MASK;
    }
    
    // Need to broadcast game whilst music is being played
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
void init_audio() {
    Serial1.begin(115200);
    // Login to raspi (this will be ignored if already logged in)
    Serial1.print("tc\n");
    delay(5);
    // Play a test sound
    audio_play(3);
}
void audio_play( int sound )
{
    Serial1.print(sound);
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
        dots[i].setPixelColor(j, dots[i].Color(255,188,150));
      }
      dots[i].show();
    }
  }  
}

void blankMap() {
  for (int i = 0; i < TOTALSTRIPS; i++) {
    for (int j = 0; j < TOTALLEDS; j+=SPACING) {
        dots[i].setPixelColor(j, dots[i].Color(0,0,0));
        dots[i].show();
    }
  }  
}
void create_leds() {
  
  byte currentState[9][9] = {
      {0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0}
  };
  int t;
  for (t = 0; t < 54; t++) {
    byte column[9];
    // Shifts in the next column of the total character grid
    shiftMapLeft(currentState, getColumn(column, startup, t));
    for (int i = 0; i < TOTALSTRIPS; i++) {
      for (int j = 0; j < TOTALLEDS; j+=SPACING) {
        if (currentState[j/SPACING][i] == 1) {
          dots[i].setPixelColor(j, dots[i].Color(30,10,180));
        } else {
           dots[i].setPixelColor(j, dots[i].Color(0,0,0));
        }
      }
      dots[i].show();
    }
    delay(100);
  }
}

void shiftMapLeft(byte grid[9][9], byte SIL[9]) {
  // SIL is a column vector for shifting in
  for (int i = 0; i  < 9; i++) {
     for (int j = 0; j < 9; j++) {
        if (j+1 < 9) {
          grid[i][j] = grid[i][j+1];
        } else {
          grid[i][j] = SIL[i];
        }
     } 
  }
}
// Returning the column just to I can embed the function
byte * getColumn(byte column[9], byte character_grid[6][9][9], int t) {
   int i;
   for (i = 0; i < 9; i++) {
     column[i] = character_grid[t/9][i][t%9];
   }
   return column;
}
