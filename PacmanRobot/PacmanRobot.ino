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
 * Map definitions
 */
#define DIM 9

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
void update_game(void);
void init_game_map(void);
void print_game(void);
/*
 * Global Variables
 */
 
static const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL};
RF24 radio(9,10);

game_state game;
// Pointer to be used when updating game from radio
game_state *g = &game;
uint8_t game_map[DIM][DIM] = {
  {R|D,   R|D|L,  R|L,    D|L,    0,     R|D,    R|L,    R|D|L,  D|L},
  {U|D,   U|R|D,  R|D|L,  U|R|L,  R|L,   U|R|L,  R|D|L,  U|D|L,  U|D},
  {U|R,   U|D|L,  U|R,    D|L,    0,     R|D,    U|L,    U|R|D,  U|L},
  {0,     U|D,    R|D,    U|R|L,  R|D|L, U|R|L,  D|L,    U|D,    0  },
  {R|D,   U|R|D|L,U|D|L,  R,      U|R|L, L,      U|R|D,  U|R|D|L,D|L},
  {U|D,   U|D,    U|R,    R|D|L,  R|L,   R|D|L,  U|L,    U|D,    U|D},
  {U|R,   U|D|L,  R|D,    U|R|D|L,R|L,   U|R|D|L,D|L,    U|R|D,  U|L},
  {R|D,   U|R|L,  U|L,    U|D,    0,     U|D,    U|R,    U|R|L,  D|L},
  {U|R,   R|L,    R|L,    U|R|L,  R|L,   U|R|L,  R|L,    R|L,    U|L}
};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  init_radio();
  Serial.println(GAME_SIZE);
  init_game(); 
}

void loop() {
  // put your main code here, to run repeatedly:
  update_game();
  // If header is wrong, return to receive update again
  if (game.header != HEADER)
    return;
  switch(game.command) {
    case STOP  : break; //Do something
    case PAUSE : break; //Do something
    case RESUME: break; //Do something
    case HIDE  : break; //Do something
    default    : break; //Do nothing 
  }
  
  
}
void print_game() {
   int i;
   for (i = 0; i < GAME_SIZE; i++) {
      Serial.print("Byte ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(*((char *)g+i), DEC);
   } 
   Serial.println();
   Serial.println();Serial.println();
}
void init_radio() {
  radio.begin();
  radio.setRetries(15,15);
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);
  radio.setChannel(BROADCAST_CHANNEL);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_MIN);
  
  radio.startListening();
}
// Initialises the game and waits for the host to send start command
void init_game() {  
  while (game.command != START) {
    //Serial.println(game.command);
    print_game();
    update_game();
    //delay(1000);
  }
  Serial.println("The game has begun!"); 
}

void update_game() {
  int i;
  if (radio.available()) {
    //Serial.println("Data available");
    radio.read((char *)g,GAME_SIZE);
  }
  
}
