#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <TimerOne.h>
#include <SPI.h>
#include <AccelStepper.h>
#include "RF24.h"

/* note: data types and variables have been grouped together for ease of reading.
   This leads to inefficient allocation of memory space, but for now we have plenty.
   Once program is working, suggest moving relevant sections into different .h and .c
   files, as well as converting as much code as possible to c++ rather than c.
   
   For now, don't move around things.
*/

/*
 * Game state definitions
 */ 
 
#define NUM_GHOSTS 3
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

#define GAME_SIZE sizeof(game_state_t)

/*
 * Map definitions
 */
#define DIM 9

#define U 0b1000
#define R 0b0100
#define D 0b0010
#define L 0b0001

// note: for error checking, we will refer to board with indices 1-9 not 0-8
// exit of "ghost cave" has been made one directional, could change easily for
// end of game or hiding mode (at x(column)=5,y(row)=4, R|L->R|D|L
static const uint8_t game_map[DIM][DIM] = {
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
static const uint8_t subDir = 0b1001;
static const uint8_t addDir = 0b0110;
static const uint8_t xx = 0b0101;
static const uint8_t yy = 0b1010;

/*
 * Radio definitions
 */

#define BROADCAST_CHANNEL 72
static const uint64_t pipe = 0xF0F0F0F0E1LL;

/*
 * Game Struct definitions
 */

typedef struct {
    uint8_t x : 4;
    uint8_t y : 4;
} position_t;

typedef char heading_t;

typedef struct {
    position_t p;
    heading_t h;
} robot_t;

// Contains information about all robots currently in play.
typedef struct {
    uint8_t header;
    uint8_t command;
    uint8_t override_dir;	
    robot_t pac;
    robot_t g[NUM_GHOSTS];
} game_state_t;

/*
 * Line sensor class and functions.
 * Yay fun c++ code.
 */

typedef enum {LEFT,CENTRE,RIGHT,NONE} line_pos_t;

class LineSensor 
{
public:
  LineSensor(uint8_t pin1,uint8_t pin2,uint8_t pin3, uint8_t pin4);
  line_pos_t get_line();
  void calibrate();
private:
  uint8_t _pins[4];
  uint16_t _min[4];
  uint16_t _max[4];
  uint16_t _threshold[4];
};

LineSensor::LineSensor(uint8_t pin1,uint8_t pin2,uint8_t pin3, uint8_t pin4){
  _pins[0] = pin1; _pins[1] = pin2; _pins[2] = pin3; _pins[3] = pin4;
  int i;
  for(i=0;i<4;i++){
    _max[i] = 0;
    _min[i] = 1023;
    _threshold[i] = 950;
  }
}

line_pos_t LineSensor::get_line(){
  int curLine[4];
  int tripCount=0, sum=0, i=0;
  for(;i<4;i++){
    curLine[i] = (analogRead(_pins[i])<_threshold[i]);
    tripCount += curLine[i];
    curLine[i]*=(2*i-3);
    sum += curLine[i];
  }
  if(tripCount==0){
    return NONE;
  } else if(sum<0){
    return LEFT;
  } else if(sum>0){
    return RIGHT;
  } else {
    return CENTRE;
  }
}

void LineSensor::calibrate(){
  int i;
  unsigned long val;
  for(i=0;i<4;i++){
    val = analogRead(_pins[i]);
    _max[i] = max(_max[i],val);
    _min[i] = min(_min[i],val);
    _threshold[i] = (_max[i]+_min[i])/2;
  }
}
/*
 * Hardware definitions
 */

//AccelStepper m_topLeft(AccelStepper::HALF4WIRE,22,24,26,28,false);
//AccelStepper m_topRight(AccelStepper::HALF4WIRE,23,25,27,29,false);
//AccelStepper m_bottomLeft(AccelStepper::HALF4WIRE,32,34,36,38,false);
//AccelStepper m_bottomRight(AccelStepper::HALF4WIRE,33,35,37,39,false);
AccelStepper m_topLeft(AccelStepper::HALF4WIRE,24,22,28,26,false);
AccelStepper m_topRight(AccelStepper::HALF4WIRE,25,23,29,27,false);
AccelStepper m_bottomLeft(AccelStepper::HALF4WIRE,34,32,38,36,false);
AccelStepper m_bottomRight(AccelStepper::HALF4WIRE,35,33,39,37,false);

LineSensor lineTop(15,14,13,12);
LineSensor lineRight(0,1,2,3);
LineSensor lineBottom(7,6,5,4);
LineSensor lineLeft(8,9,10,11);
RF24 radio(9,10);

// to be inserted: LED strip (one pin), LCD (i2c bus), button(s) (x1 or x2) for mode select

/*
 * Local definitions
 */
typedef enum {PACMAN=0, GHOST1, GHOST2, GHOST3, GHOST4} playerType_t;

// keep these consistent please
#define PLAYER GHOST2
#define PLAYER_STRING "GHOST2"
// will make this selectable via user interface on robot

#define M_SPEED 300
#define M_FAST  250
#define M_SLOW  150
#define STEPPER_MAX_SPEED 1000


/*
 * Global Variables
 */

volatile int moveFlag = 0;
playerType_t playerSelect = PLAYER;
robot_t *thisRobot;
//uint16_t robotSpeed = STEPPER_SPEED;
position_t goal;
heading_t globalHeading = '0';
heading_t currentHeading = '0';

game_state_t game;
// Extra space to prevent corruption of data, due to the required 32 byte payload.
uint8_t __space[21];
// Pointer to be used when updating game from radio
game_state_t *g = &game;


/*
 * Functions to be used by robots and hub
 */

void init_radio(void);
void init_motors(void);
void init_light_sensors(void);
void init_game(void);
void update_game(void);
void init_game_map(void);
void print_game(void);
void map_expand(void);
bool is_intersection(int x, int y);
bool is_open(int x, int y, heading_t dir);
bool collision_detect(heading_t h);
void decide_direction(uint8_t options);
void move_robot(void);
void move_flag(void);
void updateSpeed(AccelStepper *thisMotor,int newSpeed);


uint8_t* check_square(int x, int y); //SILLY that it has to be uint8_t*! BUT ARDUINO WON'T COMPILE OTHERWISE!!
uint8_t* expand(int *x,int *y, heading_t h);
uint8_t* expand_single(int *x,int *y,heading_t h);
uint8_t heading2binary(heading_t h);
void enter_robot_location(robot_t * entry);

/**    SET UP   **/

void init_radio() {
  radio.begin();
  radio.openReadingPipe(0,pipe);
  radio.setChannel(BROADCAST_CHANNEL);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setAutoAck(false);
  radio.startListening();
  radio.setPayloadSize(32);
}

void init_motors() {
  m_topLeft.setMaxSpeed(STEPPER_MAX_SPEED);
  m_topRight.setMaxSpeed(STEPPER_MAX_SPEED);
  m_bottomLeft.setMaxSpeed(STEPPER_MAX_SPEED);
  m_bottomRight.setMaxSpeed(STEPPER_MAX_SPEED);
  Timer1.initialize(100);  // 100 us hopefully fast enough for variable speeds
  Timer1.attachInterrupt( move_flag );  // interrupt calls motion directly, setting flag
                                         // is alternate option
  
}

void init_light_sensors(){
  lineTop.calibrate();
  lineRight.calibrate();
  lineBottom.calibrate();
  lineLeft.calibrate();
}


// Initialises the game and waits for the host to send start command
void init_game() {
  // To be replaced with LCD display and button toggling!
  switch(playerSelect){
    case PACMAN : thisRobot = &game.pac; break;    
    default  : thisRobot = &game.g[playerSelect-1]; break;
  }
  while (game.command != START) {
    //Serial.println(game.command);
    update_game();
    // Don't trust start command with faulty checksum
    if (game.header != checksum()) {
       game.command == STOP; 
    }
    print_game();
    //Serial.println("Will the game ever start?");
    //delay(300);
  }
  Serial.println("The game has begun!");
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(57600);
  Serial.println("Starting run...");
  init_motors();
  //pinMode(4,OUTPUT);
  //init_radio();
  //randomSeed(micros());
  //Serial.println(GAME_SIZE);
  //init_game(); 
  
}

/**    MAIN FUNCTIONS    **/
int i = 0;
void loop() {
  // put your main code here, to run repeatedly:
  /*while (1) {
    update_game();
    if (Serial.available()) {
      while (Serial.available()) {
        Serial.read();
      }
      i = 0;
      Serial.println("Reset count");
    }
    if (game.header == checksum()) {
      Serial.print("Received packets = ");
      Serial.println(++i);
      print_game();
      //delay(100); 
    }
    if (game.command == MANUAL_OVERRIDE) {
       Serial.println("Received instruction..."); 
       game.command = NOP;
       Serial.print("Dir: ");
       switch(game.override_dir) {
         case U:
           Serial.println('U');
           break;
         case D:
           Serial.println('D');
           break;
         case L:
           Serial.println('L');
           break;
         case R:
           Serial.println('R');
           break;   
       }
       Serial.println("Processed instruction.");   
    }
  }*/
  
  // If header is wrong, return to receive update again
  /*if (game.header != checksum()) {
    Serial.println("ERR: Checksum invalid");
    print_game();
    return;
  } else {
    Serial.println("Checksum passed");
  }
  switch(game.command) {
    case STOP  : break; //Do something
    case PAUSE : break; //Do something
    case RESUME: break; //Do something
    case HIDE  : break; //Do something
    default    : break; //Do nothing 
  }*/
  /*Serial.print("Welcome player ");
  Serial.println(PLAYER_STRING);
  int i;
  for(i=0;i<NUM_GHOSTS;i++){
    Serial.print("Please enter ghost ");
    Serial.println(i+1);
    enter_robot_location(&game.g[i]);
  }*/
  //map_expand();
  //collision_detect();
  if(Serial.available()){
    globalHeading=Serial.read();
  }
  if(moveFlag){
    moveFlag = 0;
    move_robot();
  }
}

void print_game() {
   int i;
   /*
   Serial.print("game.header = ");
   Serial.println(game.header);
   Serial.print("game.command = ");
   Serial.println(game.command);
   Serial.print("game.override_dir = ");
   Serial.println(game.override_dir);
   
   Serial.print("game.pac.x  = ");
   Serial.print(game.pac.p.x);
   Serial.print(" game.pac.y  = ");
   Serial.print(game.pac.p.y);
   Serial.print(" game.pac.heading  = ");
   Serial.println(game.pac.h);
   
    for (i = 0; i < NUM_GHOSTS; i++) {
       Serial.print("game.g[");
       Serial.print(i);
       Serial.print("].x = ");
       Serial.print(game.g[i].p.x);  
       Serial.print(" game.g[");
       Serial.print(i);
       Serial.print("].y = ");
       Serial.print(game.g[i].p.y);
       Serial.print(" game.g[");
       Serial.print(i);
       Serial.print("].heading = ");
       Serial.println(game.g[i].h);
    }
   */

   for (i = 0; i < GAME_SIZE; i++) {
      Serial.print("Byte \0");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(*((char *)&game+i), DEC);
   }
   Serial.println();
   Serial.println();Serial.println();
}

/**    RADIO NETWORK FUNCTIONS    **/
void update_game() {
  int i;
  if (radio.available()) {
    //Serial.println("Data available");
    //digitalWrite(4,HIGH);
    radio.read((char *)&game,GAME_SIZE);
    delay(1);
  } else {
    game.header = -1;
    //digitalWrite(4,LOW);
  }
}

uint8_t checksum() {
  uint8_t i, j, sum = 0;
  for (i = 1; i < GAME_SIZE; i++) {
     for (j = 0; j < 8; j++) {
        sum += (((char *)g)[i] & (1 << j)) >> j;
     }
  }
  return sum;
}

/**    MOVEMENT FUNCTIONS    **/

bool detect_intersection() {
  // returning true when robot has just reached an intersection
  // (to be completed)
  return true;
}

void move_flag(){
  moveFlag = 1;
}

void move_robot() {
  // use of pointers helps translate robot movement functions based on direction
  AccelStepper *m_frontLeft;
  AccelStepper *m_frontRight;
  AccelStepper *m_backLeft;
  AccelStepper *m_backRight;
  LineSensor *lineFront;
  LineSensor *lineBack;
  uint8_t directions[4];
  switch(globalHeading){
    case 'u' :
      m_frontLeft = &m_topLeft;
      m_frontRight = &m_topRight;
      m_backLeft = &m_bottomLeft;
      m_backRight = &m_bottomRight;
      lineFront = &lineTop;
      lineBack = &lineBottom;
      break;
    case 'r' :
      m_frontLeft = &m_topRight;
      m_frontRight = &m_bottomRight;
      m_backLeft = &m_topLeft;
      m_backRight = &m_bottomLeft;
      lineFront = &lineRight;
      lineBack = &lineLeft;
      break;
    case 'd' :
      //uint8_t temp = {
      m_frontLeft = &m_bottomRight;
      m_frontRight = &m_bottomLeft;
      m_backLeft = &m_topRight;
      m_backRight = &m_topLeft;
      lineFront = &lineBottom;
      lineBack = &lineTop;
      break;
    case 'l' :
      m_frontLeft = &m_bottomLeft;
      m_frontRight = &m_topLeft;
      m_backLeft = &m_bottomRight;
      m_backRight = &m_topRight;
      lineFront = &lineLeft;
      lineBack = &lineRight;
      break;
    default : 
      updateSpeed(m_frontRight,0);
      updateSpeed(m_backLeft,0);
      updateSpeed(m_frontLeft,0);
      updateSpeed(m_backRight,0);
      m_topLeft.disableOutputs();
      m_topRight.disableOutputs();
      m_bottomLeft.disableOutputs();
      m_bottomRight.disableOutputs();
      currentHeading=globalHeading;
      return; //exit here
      break;
  }
  // Will need to set speed based on relative line positioning.
  // Need to investigate whether rapid polling of set speed causes problems.
  line_pos_t readFront = lineFront->get_line();
  line_pos_t readBack = lineBack->get_line();
  if(currentHeading!=globalHeading){
    if(currentHeading=='0'){
      m_topLeft.enableOutputs();
      m_topRight.enableOutputs();
      m_bottomLeft.enableOutputs();
      m_bottomRight.enableOutputs();
    }
    currentHeading=globalHeading;
    if(readFront!=NONE&&readBack!=NONE){ // dual sensor control
      //front pair of motors
      if(readFront==RIGHT){
        updateSpeed(m_frontLeft,M_SPEED);
        updateSpeed(m_frontRight,0);
      } else if(readFront==LEFT){
        updateSpeed(m_frontLeft,0);
        updateSpeed(m_frontRight,-M_SPEED);
      } else {
        updateSpeed(m_frontLeft,M_SPEED);
        updateSpeed(m_frontRight,-M_SPEED);
      }
      //back pair of motors
      if(readBack==RIGHT){
        updateSpeed(m_backLeft,M_SPEED);
        updateSpeed(m_backRight,0);
      } else if(readBack==LEFT){
        updateSpeed(m_backLeft,0);
        updateSpeed(m_backRight,-M_SPEED);
      } else {
        updateSpeed(m_backLeft,M_SPEED);
        updateSpeed(m_backRight,-M_SPEED);
      }
    } else { // one sensor must shift whole robot
      if(readFront==RIGHT||readBack==LEFT){
        updateSpeed(m_frontLeft,M_SPEED);
        updateSpeed(m_backRight,-M_SPEED);
        updateSpeed(m_frontRight,0);
        updateSpeed(m_backRight,0);
      } else if(readFront==LEFT||readBack==RIGHT){
        updateSpeed(m_frontRight,-M_SPEED);
        updateSpeed(m_backLeft,M_SPEED);
        updateSpeed(m_frontLeft,0);
        updateSpeed(m_backRight,0);
      } else {
        //lost the line
        updateSpeed(m_frontLeft,M_SPEED);
        updateSpeed(m_backLeft,M_SPEED);
        updateSpeed(m_frontRight,-M_SPEED);
        updateSpeed(m_backRight,-M_SPEED);
      }
    }
  }
  m_frontLeft->runSpeed();
  m_frontRight->runSpeed();
  m_backLeft->runSpeed();
  m_backRight->runSpeed();
}

void updateSpeed(AccelStepper *thisMotor,int newSpeed){
  int speedVal = (int)thisMotor->speed();
  if(speedVal!=newSpeed){
    thisMotor->setSpeed(newSpeed);
  }
}

/**    MAP NAVIGATION FUNCTIONS    **/

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
  uint8_t testDir = heading2binary(dir);
  return ((square&testDir)>0);
}

bool collision_detect(heading_t h){
  robot_t *r;
  bool collision = false;
  int xDest = thisRobot->p.x;
  int yDest = thisRobot->p.y;
  if (is_intersection(xDest,yDest)){
    uint8_t testDir = heading2binary(h);
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

void decide_direction(uint8_t options){
  heading_t newHeading;
  heading_t directionList[4];
  uint8_t directionInts[4];
  float angle;
  if(goal.x==0&&goal.y==0){ //random movement mode
  }
  if(thisRobot->p.y==goal.y&&thisRobot->p.x==goal.x){
    switch(thisRobot->h){
      case 'u':
        angle = 90;
        break;
      case 'r':
        angle = 0;
        break;
      case 'd':
        angle = -90;
        break;
      case 'l':
        angle = 180;
        break;
      default :
        angle = 90;
        break;
    }
  } else {
    angle = atan2(thisRobot->p.y-goal.y,goal.x-thisRobot->p.x);
  } 
  if (angle>=135){
    strncpy(directionList,"ludr",4);
  } else if(angle>=90){
    strncpy(directionList,"ulrd",4);
  } else if(angle>=45){
    strncpy(directionList,"urld",4);
  } else if(angle>=0){
    strncpy(directionList,"rudl",4);
  } else if(angle>=-45){
    strncpy(directionList,"rdul",4);
  } else if(angle>=-90){
    strncpy(directionList,"drlu",4);
  } else if(angle>=-135){
    strncpy(directionList,"dlru",4);
  } else if(angle>=-180){
    strncpy(directionList,"ldur",4);;
  } else { // default probably not needed
    strncpy(directionList,"urdl",4);
  }
  int i;
  for(i=0;i<4;i++){
    directionInts[i] = heading2binary(directionList[i]);
  }
  i=0;
  while(i<4){
    if((options&directionInts[i])>0){
      break;
    }
    i++;
  }
  if(i==4){
    newHeading = '0';
  } else {
    newHeading = directionList[i];
  }  
}
  
void map_expand(){
  int x = thisRobot->p.x;
  int y = thisRobot->p.y;
  char h = thisRobot->h;
  uint8_t *r;
  uint8_t options = 0b0000;
  int tempX, tempY;
  // expands along 
  if(is_open(x,y,'u')&&h!='d'){
    tempX = x; tempY = y-1;
    r = expand(&tempX,&tempY,'u');
    if(r==NULL) {
      if(!collision_detect('u')){
        Serial.println("Up path available");
        options|=U;
      }
    }
  }
  if(is_open(x,y,'r')&&h!='l'){
    tempX = x+1; tempY = y;
    r = expand(&tempX,&tempY,'r');
    if(r==NULL) {
      if(!collision_detect('r')){
        Serial.println("Right path available");
        options|=R;
      }
    }
  }
  if(is_open(x,y,'d')&&h!='u'){
    tempX = x; tempY = y+1;
    r = expand(&tempX,&tempY,'d');
    if(r==NULL) {
      if(!collision_detect('d')){
        Serial.println("Down path available");
        options|=D;
      }
    }
  }
  if(is_open(x,y,'l')&&h!='r'){
    tempX = x-1; tempY = y;
    r = expand(&tempX,&tempY,'l');
    if(r==NULL) {
      if(!collision_detect('l')){
        Serial.println("Left path available");
        options|=L;
      }
    }
  }
  decide_direction(options);
}

uint8_t* expand(int *x,int *y, heading_t h){
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
    // Start potentially redundant code
    // Checks to expand along direction robot is heading first
    uint8_t testDir = heading2binary(h);
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

uint8_t* expand_single(int *x,int *y,heading_t h){
  if(*x<1||*x>9||*y<1||*y>9){
    return NULL;
  }
  uint8_t * r = NULL;//check_square(*x,*y);
  if(!is_intersection(*x,*y)){
    // Start potentially redundant code
    // Checks to expand along direction robot is heading first
    uint8_t testDir = heading2binary(h);
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

uint8_t heading2binary(heading_t h){
  uint8_t ret;
  switch(h){
    case 'u' : ret = U; break;
    case 'r' : ret = R; break;
    case 'd' : ret = D; break;
    case 'l' : ret = L; break;
    default  : ret = 0b0000; break;
  }
  return(ret);
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
