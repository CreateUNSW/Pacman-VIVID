#include <SPI.h>
#include "RF24.h"

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
#define START    1
#define STOP     2
#define HIDE     3
#define PAUSE    4
#define RESUME   5 
#define RUN_MODE 6

#define GAME_SIZE sizeof(game_state)

/*
 * Radio definitions
 */

#define BROADCAST_CHANNEL 72

/*
 * Functions to be used by robots and hub
 */

void init_radio(void);
void init_game(void);
void broadcast_game(void);

/*
 * Global Variables
 */
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
RF24 radio(9,10);

game_state game;
// Pointer to be used when updating game from radio
game_state *g = &game;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  init_radio();
  init_game();
  Serial.println("Start the game!");
}

void loop() {
  broadcast_game();
}

void init_game() {
  game.command = START;
}

void init_radio() {
  radio.begin();
  radio.setRetries(15,15);
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);
  radio.setChannel(BROADCAST_CHANNEL);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_MAX);
  
  radio.stopListening();
}

// Used by Matlab connected arduino 
void broadcast_game() {
  int i;

  for (i = 0; i < GAME_SIZE; i++) {
    //Serial.println(buf[i], DEC);
    Serial.print("Byte ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(((char *)g)[i], DEC);
  }
  Serial.println();
  radio.write((char *)g,GAME_SIZE);
  //delay(10);	
}
