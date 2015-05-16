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
RF24 radio(9,10);
const uint64_t pipe = 0xF0F0F0F0E1LL;
game_state game;
uint8_t __space[21];

void setup() {
  // put your setup code here, to run once:
  
  
  Serial.begin(57600);
  
  init_radio();
  init_game();
  Serial.println("Start the game!");
}

int i;
void loop() {
  if (Serial.available()) {    
    i = 0;
    // Delay to make sure everything has been buffered
    delay(2);
    while(Serial.available() && i < GAME_SIZE) {
        ((uint8_t *)&game)[i++] = Serial.read();      
        //Serial.write(((uint8_t *)&game)[i-1]);
    }
   
    //((uint8_t *)&game)[i] = '\0';
    //Serial.println((char *)&game);
    //Serial.println((char *)game_buf);
    delay(2);
    //for (i = 0; i < 1; i++)
    {
      broadcast_game();
    }
    delay(2);
  }
  //broadcast_game();
}  

void init_game() {
  game.command = START;
}

void init_radio() {
  Serial.print("init_radio...");
  radio.begin();
  radio.setRetries(0,0);
  radio.openWritingPipe(pipe);
  radio.setChannel(BROADCAST_CHANNEL);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.stopListening();
  radio.setAutoAck(false);
  radio.setPayloadSize(32);
  Serial.println("done");
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
  // This loop is just to test the data that is being sent
  // modifies the checksum, though this is later corrected
  /*if (i > 255*GAME_SIZE)
    i = 0;
    
  ((char *)&game)[(i++)%GAME_SIZE]++;
  */
  
  set_checksum();
  delay(2);
  radio.write((char *)&game, GAME_SIZE);
  delay(2);
}

// A simple checksum calculator, operates within 1ms
void set_checksum(void) {
  uint8_t   i, j, sum = 0;
  // Don't include checksum
  for (i = 1; i < GAME_SIZE; i++) {
    for (j = 0; j < 8; j++) {
      sum += (((char *)&game)[i] & (1 << j)) >> j;
    }
    //Serial.println(sum);
  }
  //Serial.println();
  //Serial.println();
  game.header = sum;
  
}
