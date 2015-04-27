#include <SPI.h>
#include "RF24.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
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

#define GAME_SIZE sizeof(game_state_t)

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
 * Game Struct definitions
 */

typedef struct _position_t{
    uint8_t x : 4;
    uint8_t y : 4;
} position_t;

typedef char heading_t;

typedef struct _robot_t{
    position_t p;
    heading_t h;
} robot_t;

// Contains information about all robots currently in play.
typedef struct _game_state_t{
    uint8_t header;
    uint8_t command;	
    robot_t pac;
    robot_t g1;
    robot_t g2;
    robot_t g3;
} game_state_t;

typedef enum {PACMAN, GHOST1, GHOST2, GHOST3} playerType_t;

// keep these consistent please
#define PLAYER GHOST1
#define PLAYER_STRING "GHOST1"


/*
 * Global Variables
 */
 
static const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL};
RF24 radio(9,10);

playerType_t playerSelect = PLAYER;
robot_t *thisRobot;

game_state_t game;
// Pointer to be used when updating game from radio
game_state_t *g = &game;

// note: for error checking, we will refer to board with indices 1-9 not 0-8
// exit of "ghost cave" has been made one directional, could change easily for
// end of game or hiding mode (at x(column)=5,y(row)=4, R|L->R|D|L
uint8_t game_map[DIM][DIM] = {
  {R|D,   R|D|L,  R|L,    D|L,    0,     R|D,    R|L,    R|D|L,  D|L},
  {U|D,   U|R|D,  R|D|L,  U|R|L,  R|L,   U|R|L,  R|D|L,  U|D|L,  U|D},
  {U|R,   U|D|L,  U|R,    D|L,    0,     R|D,    U|L,    U|R|D,  U|L},
  {0,     U|D,    R|D,    U|R|L,  R|L, U|R|L,  D|L,    U|D,    0  },
  {R|D,   U|R|D|L,U|D|L,  R,      U|R|L, L,      U|R|D,  U|R|D|L,D|L},
  {U|D,   U|D,    U|R,    R|D|L,  R|L,   R|D|L,  U|L,    U|D,    U|D},
  {U|R,   U|D|L,  R|D,    U|R|D|L,R|L,   U|R|D|L,D|L,    U|R|D,  U|L},
  {R|D,   U|R|L,  U|L,    U|D,    0,     U|D,    U|R,    U|R|L,  D|L},
  {U|R,   R|L,    R|L,    U|R|L,  R|L,   U|R|L,  R|L,    R|L,    U|L}
};


/*
 * Functions to be used by robots and hub
 */

void init_radio(void);
void init_game(void);
void update_game(void);
void init_game_map(void);
void print_game(void);
void map_expand(void);
uint8_t* check_square(int x, int y); //SILLY that it has to be uint8_t*! BUT ARDUINO WON'T COMPILE OTHERWISE!!
uint8_t *expand(int x,int y, heading_t h);
void enter_robot_location(robot_t * entry);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //init_radio();
  //randomSeed(micros());
  //Serial.println(GAME_SIZE);
  init_game(); 
}

void loop() {
  /*
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
  */
  Serial.print("Welcome player ");
  Serial.println(PLAYER_STRING);
  Serial.println("Please enter ghost 1");
  enter_robot_location(&game.g1);
  Serial.println("Please enter ghost 2");
  enter_robot_location(&game.g2);
  Serial.println("Please enter ghost 3");
  enter_robot_location(&game.g3);
  
  
  map_expand();
    
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
  // To be replaced with LCD display and button toggling!
  switch(playerSelect){
    case PACMAN : thisRobot = &g->pac; break;
    case GHOST1 : thisRobot = &g->g1; break;
    case GHOST2 : thisRobot = &g->g2; break;
    case GHOST3 : thisRobot = &g->g3; break;
    default  : break;
  }
  /*while (game.command != START) {
    //Serial.println(game.command);
    print_game();
    update_game();
    //delay(1000);
  }
  Serial.println("The game has begun!"); */
}

void update_game() {
  int i;
  if (radio.available()) {
    //Serial.println("Data available");
    radio.read((char *)g,GAME_SIZE);
  }
  
}

uint8_t* check_square(int x,int y){
  robot_t *r;
  if(x<1||x>9||y<1||y>9){
    r = NULL;
  } else if (g->g1.p.x==x&&g->g1.p.y==y){
    r = &g->g1;
  } else if (g->g2.p.x==x&&g->g2.p.y==y){
    r = &g->g2;
  } else if (g->g3.p.x==x&&g->g3.p.y==y){
    r = &g->g3;
  } else {
    r = NULL;
  }
  return (uint8_t*)r;
}

bool is_intersection(uint8_t square){
  int dirCount = 0;
  dirCount += (square&U)>0;
  dirCount += (square&R)>0;
  dirCount += (square&D)>0;
  dirCount += (square&L)>0;
  return (dirCount>2);
}

void map_expand(){
  int x = thisRobot->p.x;
  int y = thisRobot->p.y;
  char h = thisRobot->h;
  uint8_t *r;
  // expands along 
  if((game_map[y-1][x-1]&U)>0&&h!='d'){
    r = expand(x,y-1,'u');
    if(r==NULL) Serial.println("Up path available");
  }
  if((game_map[y-1][x-1]&R)>0&&h!='l'){
    r = expand(x+1,y,'r');
    if(r==NULL) Serial.println("Right path available");
  }
  if((game_map[y-1][x-1]&D)>0&&h!='u'){
    r = expand(x,y+1,'d');
    if(r==NULL) Serial.println("Down path available");
  }
  if((game_map[y-1][x-1]&L)>0&&h!='r'){
    r = expand(x-1,y,'l');
    if(r==NULL) Serial.println("Left path available");
  }
}

uint8_t *expand(int x,int y, heading_t h){
  // Debug output
  Serial.print("Checking: ");
  Serial.print(x); Serial.print(" "); Serial.println(y);
  if(x<1||x>9||y<1||y>9){
    return NULL;
  }
  uint8_t * r = check_square(x,y);
  if(r!=NULL){
    // return r;
  } else if(!is_intersection(game_map[y-1][x-1])){
    if((game_map[y-1][x-1]&U)>0&&h!='d'){
      r = expand(x,y-1,'u');
    } else if((game_map[y-1][x-1]&R)>0&&h!='l'){
      r = expand(x+1,y,'r');
    } else if((game_map[y-1][x-1]&D)>0&&h!='u'){
      r = expand(x,y+1,'d');
    } else if((game_map[y-1][x-1]&L)>0&&h!='r'){
      r = expand(x-1,y,'l');
    }
  }
  return r;
  
}
// test function only
void enter_robot_location(robot_t * entry){
  int x;
  int y;
  char temp;
  heading_t h;
  Serial.println("Please enter x y heading in format xyh");
  while(!Serial.available()){}
  temp = Serial.read();
  x = atoi(&temp);
  Serial.print(x);
  while(!Serial.available()){}
  temp = Serial.read();
  y = atoi(&temp);
  Serial.print(y);
  while(!Serial.available()){}
  h = Serial.read();
  Serial.println(h);
  //Test locations
  entry->p.x = x;
  entry->p.y = y;
  entry->h = h;
}

    
  


