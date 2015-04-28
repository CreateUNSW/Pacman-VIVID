#include <SPI.h>
#include "RF24.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
/*
 * Game state definitions
 */ 
 
#define NUM_GHOSTS 3
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
    robot_t g[NUM_GHOSTS];
} game_state_t;

typedef enum {PACMAN=0, GHOST1, GHOST2, GHOST3, GHOST4} playerType_t;

// keep these consistent please
#define PLAYER GHOST2
#define PLAYER_STRING "GHOST2"


/*
 * Global Variables
 */
 
static const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL};
RF24 radio(9,10);

playerType_t playerSelect = PLAYER;
robot_t *thisRobot;

game_state_t game;
// Pointer to be used when updating game from radio
//game_state_t *g = &game;

// note: for error checking, we will refer to board with indices 1-9 not 0-8
// exit of "ghost cave" has been made one directional, could change easily for
// end of game or hiding mode (at x(column)=5,y(row)=4, R|L->R|D|L
const uint8_t game_map[DIM][DIM] = {
  {R|D,   R|D|L,  R|L,    D|L,    0,     R|D,    R|L,    R|D|L,  D|L},
  {U|D,   U|R|D,  R|D|L,  U|R|L,  R|L,   U|R|L,  R|D|L,  U|D|L,  U|D},
  {U|R,   U|D|L,  U|R,    D|L,    0,     R|D,    U|L,    U|R|D,  U|L},
  {0,     U|D,    R|D,    U|R|L,  R|L,   U|R|L,  D|L,    U|D,    0  },
  {R|D,   U|R|D|L,U|D|L,  R,      U|R|L, L,      U|R|D,  U|R|D|L,D|L},
  {U|D,   U|D,    U|R,    R|D|L,  R|L,   R|D|L,  U|L,    U|D,    U|D},
  {U|R,   U|D|L,  R|D,    U|R|D|L,R|L,   U|R|D|L,D|L,    U|R|D,  U|L},
  {R|D,   U|R|L,  U|L,    U|D,    0,     U|D,    U|R,    U|R|L,  D|L},
  {U|R,   R|L,    R|L,    U|R|L,  R|L,   U|R|L,  R|L,    R|L,    U|L}
};

//Variables for smart map expansion
const uint8_t subDir = 0b1001;
const uint8_t addDir = 0b0110;
const uint8_t xx = 0b0101;
const uint8_t yy = 0b1010;


/*
 * Functions to be used by robots and hub
 */

void init_radio(void);
void init_game(void);
void update_game(void);
void init_game_map(void);
void print_game(void);
void map_expand(void);
bool is_intersection(int x, int y);
bool is_open(int x, int y, heading_t dir);

uint8_t* check_square(int x, int y); //SILLY that it has to be uint8_t*! BUT ARDUINO WON'T COMPILE OTHERWISE!!
uint8_t *expand(int *x,int *y, heading_t h);
uint8_t *expand_single(int *x,int *y,heading_t h);
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
  int i;
  for(i=0;i<NUM_GHOSTS;i++){
    Serial.print("Please enter ghost ");
    Serial.println(i+1);
    enter_robot_location(&game.g[i]);
  }
  map_expand();
  //collision_detect();
    
}

void print_game() {
   int i;
   for (i = 0; i < GAME_SIZE; i++) {
      Serial.print("Byte ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(*((char *)&game+i), DEC);
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
    case PACMAN : thisRobot = &game.pac; break;    
    default  : thisRobot = &game.g[playerSelect-1]; break;
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
    radio.read((char *)&game,GAME_SIZE);
  }
  
}

uint8_t* check_square(int x,int y){
  robot_t *r = NULL;
  if(x<1||x>9||y<1||y>9){
    r = NULL;
  }/* else if (game.pac.p.x==x&&game.pac.p.y==y){
    r = &game.pac;
  } */ // probably shouldn't consider pacman's location
  else {
    int i;
    for(i=0;i<NUM_GHOSTS;i++){
      if (game.g[i].p.x==x&&game.g[i].p.y==y){
        r = &game.g[i];
        break;
      }
    }
  }
  return (uint8_t*)r;
}

bool is_intersection(int x, int y){
  if(x<1||x>9||y<1||y>9){
    return false;
  }
  uint8_t square = game_map[y-1][x-1];
  int dirCount = 0;
  dirCount += (square&U)>0;
  dirCount += (square&R)>0;
  dirCount += (square&D)>0;
  dirCount += (square&L)>0;
  return (dirCount>2);
}

bool is_open(int x, int y, heading_t dir){
  if(x<1||x>9||y<1||y>9){
    return false;
  }
  uint8_t square = game_map[y-1][x-1];
  uint8_t testDir;
  switch(dir){
    case 'u' : testDir = U; break;
    case 'r' : testDir = R; break;
    case 'd' : testDir = D; break;
    case 'l' : testDir = L; break;
    default  : testDir = 0b1111; break;
  }
  return ((square&testDir)>0);
}

bool collision_detect(){
  robot_t *r;
  bool collision = false;
  int xDest = thisRobot->p.x;
  int yDest = thisRobot->p.y;
  heading_t h = thisRobot->h;
  if (is_intersection(xDest,yDest)){
    uint8_t testDir;
    switch(h){
      case 'u' : testDir = U; break;
      case 'r' : testDir = R; break;
      case 'd' : testDir = D; break;
      case 'l' : testDir = L; break;
      default  : testDir = R; break;
    }
    // force look at the next intersection
    if(is_open(xDest,yDest,h)){
      xDest = xDest + ((xx&testDir&addDir)>0) - ((xx&testDir&subDir)>0);
      yDest = yDest + ((yy&testDir&addDir)>0) - ((yy&testDir&subDir)>0);
    }
  }
  expand_single(&xDest,&yDest,h);
  // possibility to expand a second time if need be
  if (!is_intersection(xDest,yDest)){
    return false; // if we're not about to reach intersection, don't worry!
    // may want to alter behaviour to keep spacing in future
  }
  int i;
  int xDestCheck, yDestCheck;
  heading_t hCheck;
  for(i=0;i<NUM_GHOSTS;i++){
    if(i!=playerSelect-1){ //don't check with yourself
      r = &game.g[i];
      xDestCheck = r->p.x;
      yDestCheck = r->p.y;
      hCheck = r->h;
      if(is_intersection(xDestCheck,yDestCheck)){
        if(xDestCheck==xDest&&yDestCheck==yDest){
          //stop our ghost robot from colliding!
          //need to insert relevant action (i know what i want to do, just have to do it)
          Serial.println("Stop the bot!");
          collision = true;
          break;
        }
      } else {
        expand_single(&xDestCheck,&yDestCheck,hCheck);
        if(xDestCheck==xDest&&yDestCheck==yDest){
          if(i<playerSelect-1){
            //the incoming ghost is a higher priority! let them go first!
            Serial.println("Stop the bot!");
            collision = true;
            break;
          } else {
            // we want to go through the intersection first, assume they will stop
          }
        }
      }
    }
  }
  return collision;
}
  
void map_expand(){
  int x = thisRobot->p.x;
  int y = thisRobot->p.y;
  char h = thisRobot->h;
  uint8_t *r;
  int tempX, tempY;
  // expands along 
  if(is_open(x,y,'u')&&h!='d'){
    tempX = x; tempY = y-1;
    r = expand(&tempX,&tempY,'u');
    if(r==NULL) {
      thisRobot->h = 'u';
      if(!collision_detect()){
        Serial.println("Up path available");
      }
    }
  }
  if(is_open(x,y,'r')&&h!='l'){
    tempX = x+1; tempY = y;
    r = expand(&tempX,&tempY,'r');
    if(r==NULL) {
      thisRobot->h = 'r';
      if(!collision_detect()){
        Serial.println("Right path available");
      }
    }
  }
  if(is_open(x,y,'d')&&h!='u'){
    tempX = x; tempY = y+1;
    r = expand(&tempX,&tempY,'d');
    if(r==NULL) {
      thisRobot->h = 'd';
      if(!collision_detect()){
        Serial.println("Down path available");
      }
    }
  }
  if(is_open(x,y,'l')&&h!='r'){
    tempX = x-1; tempY = y;
    r = expand(&tempX,&tempY,'l');
    if(r==NULL) {
      thisRobot->h = 'l';
      if(!collision_detect()){
        Serial.println("Left path available");
      }
    }
  }
}

uint8_t *expand(int *x,int *y, heading_t h){
  // Debug output
  Serial.print("Checking: ");
  Serial.print(*x); Serial.print(" "); Serial.println(*y);
  if(*x<1||*x>9||*y<1||*y>9){
    return NULL;
  }
  uint8_t * r = check_square(*x,*y);
  if(r!=NULL){
    // return r;
    if(r==(uint8_t*)&game.pac){
      r=NULL;
    }
  } else if(!is_intersection(*x,*y)){
    // Start potentially redundant code]
    // Checks to expand along direction robot is heading first
    uint8_t testDir;
    switch(h){
      case 'u' : testDir = U; break;
      case 'r' : testDir = R; break;
      case 'd' : testDir = D; break;
      case 'l' : testDir = L; break;
      default  : testDir = R; break;
    }
    if(is_open(*x,*y,h)){
      *x = *x + ((xx&testDir&addDir)>0) - ((xx&testDir&subDir)>0);
      *y = *y + ((yy&testDir&addDir)>0) - ((yy&testDir&subDir)>0);
      r = expand(x,y,h);
    } // end potentially redundant code 
    else if(is_open(*x,*y,'u')&&h!='d'){
      *y = *y-1;
      r = expand(x,y,'u');
    } else if(is_open(*x,*y,'r')&&h!='l'){
      *x = *x+1;
      r = expand(x,y,'r');
    } else if(is_open(*x,*y,'d')&&h!='u'){
      *y = *y+1;
      r = expand(x,y,'d');
    } else if(is_open(*x,*y,'l')&&h!='r'){
      *x = *x+1;
      r = expand(x,y,'l');
    }
  }
  return r;
  
}

uint8_t *expand_single(int *x,int *y,heading_t h){
  if(*x<1||*x>9||*y<1||*y>9){
    return NULL;
  }
  uint8_t * r = NULL;//check_square(*x,*y);
  if(!is_intersection(*x,*y)){
    // Start potentially redundant code]
    // Checks to expand along direction robot is heading first
    uint8_t testDir;
    switch(h){
      case 'u' : testDir = U; break;
      case 'r' : testDir = R; break;
      case 'd' : testDir = D; break;
      case 'l' : testDir = L; break;
      default  : testDir = R; break;
    }
    if(is_open(*x,*y,h)){
      *x = *x + ((xx&testDir&addDir)>0) - ((xx&testDir&subDir)>0);
      *y = *y + ((yy&testDir&addDir)>0) - ((yy&testDir&subDir)>0);
      r = check_square(*x,*y);
    } // end potentially redundant code 
    else if(is_open(*x,*y,'u')&&h!='d'){
      *y = *y-1;
      r = check_square(*x,*y);
    } else if(is_open(*x,*y,'r')&&h!='l'){
      *x = *x+1;
      r = check_square(*x,*y);
    } else if(is_open(*x,*y,'d')&&h!='u'){
      *y = *y+1;
      r = check_square(*x,*y);
    } else if(is_open(*x,*y,'l')&&h!='r'){
      *x = *x+1;
      r = check_square(*x,*y);
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

    
  


