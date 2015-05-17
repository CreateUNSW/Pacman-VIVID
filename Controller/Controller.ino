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
} position_t;

typedef char heading_t;

typedef struct _robot {
    position_t p;
    heading_t h;
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
 * Controller definitions
 */
 
#define U 0b1000
#define R 0b0100
#define D 0b0010
#define L 0b0001

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
 // Only need a single pipe due to one-way comms.
const uint64_t pipe = 0xF0F0F0F0E1LL;
RF24 radio(48,49);

game_state game;
uint8_t __space[21];
// Pointer to be used when updating game from radio
// Pin definitions
int left  = 2;
int up    = 3;
int right = 4;
int down  = 5;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(57600);
  init_radio();
  //init_game();
  pinMode(up   , INPUT);
  pinMode(down , INPUT);
  pinMode(left , INPUT);
  pinMode(right, INPUT);
  Serial.println("Now receiving inputs!");
}

void loop() {
  game.command = MANUAL_OVERRIDE;
  // Read button presses
  if (digitalRead(up)) 
    game.override_dir = U;
  else if (digitalRead(down))
    game.override_dir = D;
  else if (digitalRead(left))
    game.override_dir = L;
  else if (digitalRead(right))
    game.override_dir = R;
  else {
    game.override_dir = 0;
    game.command = NOP;
  }
  
  if (game.command == MANUAL_OVERRIDE) {
    // Send game information
    //int time = millis();
    Serial.println("I am transmitting something");
    broadcast_game();
    //Serial.println(millis() - time);
    //Serial.println(game.override_dir);
  }

  //Serial.println("Now start over again!");
}

void init_game() {
  // All data should be zeroed out to create valid checksum  
  game.command = NOP;
  game.override_dir = 0;
  game.pac = {0,0};
  game.g1 =  {0,0};
  game.g2 =  {0,0};
  game.g3 =  {0,0};
}

void init_radio() {
  radio.begin();
  radio.setRetries(15,15);
  radio.openWritingPipe(pipe);
  radio.setChannel(BROADCAST_CHANNEL);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.stopListening();
  radio.setAutoAck(false);
  radio.setPayloadSize(32);
}

// Used by Matlab connected arduino 
void broadcast_game() {
  int i;
  set_checksum();
  radio.write((char *)&game,GAME_SIZE); 
}

// A simple checksum calculator, operates within 1ms
void set_checksum(void) {
  uint8_t  i, j, sum = 0;
  // Don't include checksum
  for (i = 1; i < GAME_SIZE; i++) {
    for (j = 0; j < 8; j++) {
      sum += (((char *)&game)[i] & (1 << j)) >> j;
      /*Serial.print("i:");
      Serial.print(i);
      Serial.print(" j:");
      Serial.print(j);
      Serial.print(" sum:");
      Serial.println(sum);
     */ 
    }
  }
  game.header = sum;
  
}
