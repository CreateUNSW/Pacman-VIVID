//#define DEBUG_RF   // Debug messages related to updating the game state + internals from RF
//#define DEBUG_LIN  // Debug messages related to line following and sensors
//#define DEBUG_MOT  // Debug messages related to motor control
//#define DEBUG_AI   // Debug messages related to AI decisions
//#define DEBUG_MAN  // Debug messages related to manual control decisions

uint8_t red = 0;
uint8_t blue = 0;
uint8_t green = 0;

void debug_colour();

//#define USE_RADIO
  
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <TimerOne.h>
#include <SPI.h>
#include <AccelStepper.h>
#include "RF24.h"
#include <Adafruit_NeoPixel.h>
#include <avr/power.h>

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
#define NOP             B0000000
#define START           B0000001
#define STOP            B0000010
#define HIDE            B0000100
#define PAUSE           B0001000
#define RESUME          B0010000 
#define MUSIC_COMMAND   B0100000
#define MANUAL_OVERRIDE B1000000

#define GAME_SIZE sizeof(game_state_t)

// Pacman death sequence code, must NOT conflict with music mask of broadcaster
#define PAC_DEATH 0b10000000
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
  void init_calibrate();
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

void LineSensor::init_calibrate(){
  int i;
  for(i=0;i<4;i++){
    _max[i] = 0;
    _min[i] = 1023;
    _threshold[i] = 950;
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
 
// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define LED_STRIP_PIN            43
// eyes connected to pins 42,44,46,48
#define EYE_LL  42
#define EYE_LR  44
#define EYE_RL  46
#define EYE_RR  48

uint8_t rgb[3];
const uint8_t pacStrip [4][5] = {{0,1,8,9,10},{2,3,11,12,13},{4,5,14,15,16},{6,7,17,18,19}};
// top, right, bottom, left region

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      20// (max for Pacman?)
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LED_STRIP_PIN, NEO_GRB + NEO_KHZ800);

AccelStepper m_topLeft(AccelStepper::HALF4WIRE,26,28,22,24,false);
AccelStepper m_topRight(AccelStepper::HALF4WIRE,27,29,23,25,false);
AccelStepper m_bottomLeft(AccelStepper::HALF4WIRE,36,38,32,34,false);
AccelStepper m_bottomRight(AccelStepper::HALF4WIRE,37,39,33,35,false);

LineSensor lineTop(3,2,1,0);
LineSensor lineLeft(7,6,5,4);
LineSensor lineBottom(11,10,9,8);
LineSensor lineRight(15,14,13,12);

RF24 radio(49,53);

// to be inserted: button(s) (x1 or x2) for mode select

/*
 * Local definitions
 */
typedef enum {PACMAN=0, GHOST1, GHOST2, GHOST3, GHOST4} playerType_t;

// keep these consistent please
#define PLAYER PACMAN
//#define PLAYER_STRING "GHOST1"
// will make this selectable via user interface on robot

#define M_SPEED 300
#define M_FAST  500
#define M_SLOW  150
#define STEPPER_MAX_SPEED 1000
#define MAX_ALIGN_TIME 2000

/*
 * Global Variables
 */

volatile int moveFlag = 0;
playerType_t playerSelect = PLAYER;
robot_t *thisRobot;
//uint16_t robotSpeed = STEPPER_SPEED;
position_t goal;
heading_t globalHeading;
volatile heading_t currentHeading;
unsigned long aiTime = 0;
unsigned long loopTime = 0;
unsigned long lastIntersection = 0;
bool animationToggle = 0;

game_state_t game;
// Extra space to prevent corruption of data, due to the required 32 byte payload.
uint8_t __space[21];

// Initialise the robots as stopped
int curr_command = STOP;


/*
 * Functions to be used by robots and hub
 */

void init_radio(void);
void init_motors(void);
void init_light_sensors(void);
void calibrate_light_sensors(void);
void init_LEDs(void);

void ghost_animation(void);
void pac_animation(void);

void init_game(void);
void update_game(void);
void init_game_map(void);
void print_game(void);
void set_checksum(void);
uint8_t checksum(void);


void move_robot(void);
void move_flag(void);
void updateSpeed(AccelStepper *thisMotor,int newSpeed);
void get_motor_alignment(AccelStepper **m_fL,AccelStepper **m_fR,AccelStepper **m_bL,AccelStepper **m_bR);
void get_line_alignment(LineSensor **l_F,LineSensor **l_B);
void line_follow(void);
bool align2intersection(void);

void set_goal(void);
void map_expand(void);
bool is_intersection(int x, int y);
bool is_open(int x, int y, heading_t dir);
bool collision_detect(heading_t h);
void decide_direction(uint8_t options);
uint8_t* check_square(int x, int y); //SILLY that it has to be uint8_t*! BUT ARDUINO WON'T COMPILE OTHERWISE!!
uint8_t* expand(int *x,int *y, heading_t h);
uint8_t* expand_single(int *x,int *y,heading_t h);
uint8_t heading2binary(heading_t h);
void enter_robot_location(robot_t * entry);
void calc_new_square(int *x,int*y,heading_t h,int d);

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
  m_topLeft.disableOutputs();
  m_topRight.disableOutputs();
  m_bottomLeft.disableOutputs();
  m_bottomRight.disableOutputs();
  Timer1.initialize(100);  // 100 us hopefully fast enough for variable speeds
  Timer1.attachInterrupt( move_robot );  // interrupt sets motion flag
}

void init_light_sensors(){
  lineTop.init_calibrate();
  lineRight.init_calibrate();
  lineBottom.init_calibrate();
  lineLeft.init_calibrate();
}

void calibrate_light_sensors(){
  lineTop.calibrate();
  lineRight.calibrate();
  lineBottom.calibrate();
  lineLeft.calibrate();
}

void init_LEDs(){
  pixels.begin();
  pinMode(EYE_LL,OUTPUT);
  pinMode(EYE_LR,OUTPUT);
  pinMode(EYE_RL,OUTPUT);
  pinMode(EYE_RR,OUTPUT);
  int i;
  for(i=0;i<NUMPIXELS;i++){
     pixels.setPixelColor(i, pixels.Color(0,0,0));
  }
  pixels.show();
}

void debug_colour() {
  int i;
  for(i=0;i<NUMPIXELS;i++) {
    pixels.setPixelColor(i, pixels.Color(red,green,blue));
  }
  pixels.show();
}

void player_LEDs(){
  switch(playerSelect){
    case PACMAN :
      rgb[0] = 220;
      rgb[1] = 220;
      rgb[2] = 0;
      break;
    case GHOST1 :
      rgb[0] = 200;
      rgb[1] = 0;
      rgb[2] = 0;
      break;
    case GHOST2 :
      rgb[0] = 0;
      rgb[1] = 0;
      rgb[2] = 200;
      break;
    case GHOST3 :
      rgb[0] = 150;
      rgb[1] = 60;
      rgb[2] = 60;
      break;
    default :
      return;
      break;
  }
  int i;
  for(i=0;i<NUMPIXELS;i++){
     pixels.setPixelColor(i, pixels.Color(rgb[0],rgb[1],rgb[2]));
  }
  pixels.show();
  
}

void pac_animation(){
  uint8_t pacLEDsegment;
  int i;
  for(i=0;i<20;i++){
    pixels.setPixelColor(i, pixels.Color(rgb[0],rgb[1],rgb[2]));
  }
  if(animationToggle==0){
    animationToggle = 1;
  } else {
    animationToggle = 0;
    switch(globalHeading){
      case 'u':
        pacLEDsegment = 0;
        break;
      case 'r':
        pacLEDsegment = 1;
        break;
      case 'd':
        pacLEDsegment = 2;
        break;
      case 'l':
        pacLEDsegment = 3;
        break;
      default:
        pixels.show();
        return;
        break;
    }
    for(i=0;i<5;i++){
      pixels.setPixelColor(pacStrip[pacLEDsegment][i],0,0,0);
    }
  }
  pixels.show();
}
              
void ghost_animation(){
  heading_t ledHeading = globalHeading;
  if(ledHeading=='u'||ledHeading=='d'){
    if(animationToggle==0){
      animationToggle = 1;
      ledHeading='r';
    } else {
      animationToggle = 0;
      ledHeading='l';
    }
  }
  if(ledHeading=='r'){
    digitalWrite(EYE_LR,HIGH);
    digitalWrite(EYE_RR,HIGH);
    digitalWrite(EYE_LL,LOW);
    digitalWrite(EYE_RL,LOW);
  } else if(ledHeading=='l'){
    digitalWrite(EYE_LL,HIGH);
    digitalWrite(EYE_RL,HIGH);
    digitalWrite(EYE_LR,LOW);
    digitalWrite(EYE_RR,LOW);
  } else {
    digitalWrite(EYE_LR,HIGH);
    digitalWrite(EYE_RL,HIGH);
    digitalWrite(EYE_LL,LOW);
    digitalWrite(EYE_RR,LOW);
  }
}
  

// Initialises the game and waits for the host to send start command
void init_game() {
  // To be replaced with LCD display and button toggling!
  switch(playerSelect){
    case PACMAN : thisRobot = &game.pac; break;    
    default  : thisRobot = &game.g[playerSelect-1]; break;
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(57600);
//  Serial.println("Starting run...");
  globalHeading = 'r';
  currentHeading = globalHeading;
  init_LEDs();
  //while(!Serial.available()){}
  init_motors();
  init_light_sensors();
  randomSeed(micros());
  #ifdef USE_RADIO
    init_radio();
  #endif
  // purple
  red = 50; green = 0; blue = 50;
  debug_colour();
  pinMode(47, OUTPUT);
  digitalWrite(47,LOW);
  pinMode(45,INPUT_PULLUP);
  // Wait for first button press
  while (digitalRead(45));
  delay(50);
  while (!digitalRead(45));
  // software debounce with this delay
  // blue
  red = 0; green = 0; blue = 50;
  debug_colour();
  delay(50);
  // Wait for second button press and continually calibrate
  while (digitalRead(45)) {
    calibrate_light_sensors();
  }
  delay(50);
  while (!digitalRead(45));
  
  //Serial.println(GAME_SIZE);
  //init_game();
  
  player_LEDs(); 
  
}

/**    MAIN FUNCTIONS    **/

unsigned long manual_override_timer = 0;
heading_t override_dir = globalHeading;
// A simple count to "hold" a person's input
// NOTE: This simply prevents the return from manual control to AI driven for a period of time, the
//       game is still updated normally, including if the person changes their input
#define MANUAL_OVERRIDE_THRESHOLD 10000
void loop() {
  if (!digitalRead(45)) {
    delay(50);
    while (!digitalRead(45));
    delay(50);
    setup();
  }
  #ifdef USE_RADIO
    update_game();
    // Whilst the last manual_override is in effect, don't update the current command
    if (curr_command == MANUAL_OVERRIDE && (millis() - manual_override_timer) >= MANUAL_OVERRIDE_THRESHOLD) {
      // Once threshold reached, return to standard play (w/ AI)
      manual_override_timer = 0;
      curr_command = NOP;
      #ifdef DEBUG_RF
        Serial.println("RF: MANUAL_OVERRIDE IN EFFECT");
      #endif
    }
    // Make sure the game header is consistent with the checksum 
    if (game.header == checksum()) {
      // This section updates all relevant variables from the received RF packet
      switch(game.command) {
        case NOP   : // No special operation, run game as normal
          // Do not permit normal operations if manual override, pause or stop are in effect
          if (curr_command == MANUAL_OVERRIDE || curr_command == STOP || curr_command == PAUSE) { 
            #ifdef DEBUG_RF
              Serial.println("RF: NOP UNAVAILABLE");
            #endif
            break;
          }
          #ifdef DEBUG_RF
            Serial.println("RF: NOP ISSUED");
          #endif
          curr_command = NOP;
          break;
        case START : // Begin the game from the initial position after the end of a game
          // Cannot start game unless robots are in the STOP state. (STOP can be issued at any time)
          if (curr_command != STOP) {
             #ifdef DEBUG_RF
               Serial.println("RF: START UNAVAILABLE");
             #endif
             break; 
          }
          // Turn on all robots, ghosts must cascade out of middle section
          // Return to normal play
          // NOTE: Not sure if you want to do special motor control stuff using the START command
          //       if so, just change this to START and add a case to the very next switch statement
          
          curr_command = START;
          #ifdef DEBUG_RF
            Serial.println("RF: START ISSUED");
          #endif
          break;
        case STOP  : // To be sent when the game is over
          if (game.override_dir & PAC_DEATH == PAC_DEATH) {
            #ifdef DEBUG_RF
              Serial.println("RF: PACMAN DIED");
            #endif  
            // Call pacman death sequence
          }
          // Turn off all robots
          // Remain stopped
          #ifdef DEBUG_RF
            Serial.println("RF: STOP ISSUED");
          #endif
          curr_command = STOP;
          break; 
        case PAUSE : // Pause the game temporarily
          // Prevents sneaky resume of gameplay (STOP -> PAUSE -> RESUME, would be valid otherwise)
          if (curr_command == STOP) {
            #ifdef DEBUG_RF
              Serial.println("RF: PAUSE UNAVAILABLE");
            #endif
            break;
          }
          #ifdef DEBUG_RF
            Serial.println("RF: PAUSE ISSUED");
          #endif
          // Turn off all robot's motors, probably not the lights
          // Remain paused until resume issued
          curr_command = PAUSE;
          break;
        case RESUME:  // Resume from a pause
          // Turn on all robot's motors
          // CANNOT resume from a stop
          if (curr_command == STOP) {
            #ifdef DEBUG_RF
              Serial.println("RF: RESUME UNAVAILABLE");
            #endif
            break;
          }
          #ifdef DEBUG_RF
            Serial.println("RF: RESUME ISSUED");
          #endif
          // Return to normal play
          curr_command = NOP;
          break;
        case MANUAL_OVERRIDE  :
          // Can't control while paused or stopped.
          if (curr_command == STOP || curr_command == PAUSE) {
            #ifdef DEBUG_RF
              Serial.println("RF: MANUAL_OVERRIDE UNAVAILABLE");
            #endif
            break;
          }
          // Can't manually control ghosts
          if (playerSelect != PACMAN)
            break;
          #ifdef DEBUG_RF
            Serial.println("RF: MANUAL_OVERRIDE ISSUED");
          #endif
          // Set current time, only use manual override
          manual_override_timer = millis();
          curr_command = MANUAL_OVERRIDE;
          
        default    : 
          #ifdef DEBUG_RF
            Serial.println("RF: INVALID COMMAND");
          #endif
          break; //Do nothing 
      }
    } else {
      #ifdef DEBUG_RF
        Serial.println("RF: INVALID CHECKSUM");
      #endif
    } 
    
    // The meat of controlling the motors + line sensing will be done here
    switch(curr_command) {
      case NOP             : /*Execute normal AI control functions*/ break;
      case MANUAL_OVERRIDE : /*Use game.override_dir (U, D, L, R) to decide your next move*/ break;
      case PAUSE           : /*Continue to the next case...*/
      case STOP            : /*If button pressed, change player*/ break;
      default              : /*PAUSE, STOP, START*/ break;
    }
  #else  
    // Non-map based motor control goes here
    //red = 0; green = 0; blue = 0;
    //debug_colour();
   // decide_direction(detect_intersection());
    uint8_t options;
    if ((options = detect_intersection()) > 0) {
      decide_direction(options);
    }
    line_follow();
    /*if(moveFlag){
      moveFlag = 0;
      move_robot();
    }*/
  #endif
  
  if(millis()-loopTime>500){
    if (playerSelect != PACMAN) {
      ghost_animation();
    } else {
      pac_animation();
    }
    loopTime = millis();
  }
  // Old code just in case you want a reminder.
  //map_expand();
  //collision_detect();
//  if(Serial.available()){
//    globalHeading=Serial.read();
//  }
//  if(moveFlag){
//    moveFlag = 0;
//    move_robot();
//  }

}

void print_game() {
   int i;
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
    game_state_t game_buf;
    radio.read((char *)&game_buf,GAME_SIZE);
    if (game_buf.header == checksum() && game_buf.command == MANUAL_OVERRIDE) {
       game.command = MANUAL_OVERRIDE;
       game.override_dir = game_buf.override_dir;
       // Do NOT overwrite current pac and ghost locations and headings
       // A REALLY dodgy way to keep the game consistent, because the controller
       // doesn't have robot information
       set_checksum();
    } else if (game_buf.header == checksum()) {
      // For normal game update, 
      game = game_buf;
    }
  } else {
    // Not achievable with checksum()
    game.header = -1;
  }
}

uint8_t checksum() {
  uint8_t i, j, sum = 0;
  for (i = 1; i < GAME_SIZE; i++) {
     for (j = 0; j < 8; j++) {
        sum += (((char *)&game)[i] & (1 << j)) >> j;
     }
  }
  return sum;
}

void set_checksum() {
  uint8_t i, j, sum = 0;
  for (i = 1; i < GAME_SIZE; i++) {
     for (j = 0; j < 8; j++) {
        sum += (((char *)&game)[i] & (1 << j)) >> j;
     }
  }
  game.header = sum;
}

/**    MOVEMENT FUNCTIONS    **/

uint8_t detect_intersection() {
  // returning true when robot has just reached an intersection
  // (to be completed)
  if((millis()-lastIntersection)<2000){
    return 0;
  }
  line_pos_t readTop = lineTop.get_line();
  line_pos_t readRight = lineRight.get_line();
  line_pos_t readBottom = lineBottom.get_line();
  line_pos_t readLeft = lineLeft.get_line();
  uint8_t trip = (readTop!=NONE)+(readRight!=NONE)+(readBottom!=NONE)+(readLeft!=NONE);
  if (trip<2) {
    return 0;
  } else if(trip>2) {
    // red
    //red = 50; green = 0; blue = 0;
    //debug_colour();
    unsigned long time = millis();
    while(!align2intersection() && (millis() - time < MAX_ALIGN_TIME)){}
  } else if(readTop!=NONE&&readBottom!=NONE){
    return 0;
  } else if(readLeft!=NONE&&readRight!=NONE){
    return 0;
  } else {
    unsigned long time = millis();
    while(!align2intersection() && (millis() - time < MAX_ALIGN_TIME)){}
  }  
  uint8_t options = 0;
  readTop = lineTop.get_line();
  readRight = lineRight.get_line();
  readBottom = lineBottom.get_line();
  readLeft = lineLeft.get_line();
  trip = (readTop!=NONE)+(readRight!=NONE)+(readBottom!=NONE)+(readLeft!=NONE);
  if (trip<2) {
    return 0;
  } else if(trip>2) {
    //red = 50; green = 0; blue = 0;
    //debug_colour();
    if(readTop!=NONE){
      options|=U;
    }
    if(readRight!=NONE){
      options|=R;
    }
    if(readBottom!=NONE){
      options|=D;
    }
    if(readLeft!=NONE){
      options|=L;
    }
    lastIntersection = millis();
    return options;
  } else if(readTop!=NONE&&readBottom!=NONE){
    return 0;
  } else if(readLeft!=NONE&&readRight!=NONE){
    return 0;
  } else {
    //at corner, switches heading directly
    if(readTop!=NONE&&globalHeading!='d'){
      globalHeading='u';
    } else if(readRight!=NONE&&globalHeading!='l'){
      globalHeading='r';
    } else if(readBottom!=NONE&&globalHeading!='u'){
      globalHeading='d';
    } else if(readLeft!=NONE&&globalHeading!='r'){
      globalHeading='l';
    }
    lastIntersection = millis();
    return 0;
  }
}

bool align2intersection() {
  int m_tL_speed = 0;
  int m_tR_speed = 0;
  int m_bL_speed = 0;
  int m_bR_speed = 0;
  bool ret = true;
  line_pos_t readTop = lineTop.get_line();
  line_pos_t readRight = lineRight.get_line();
  line_pos_t readBottom = lineBottom.get_line();
  line_pos_t readLeft = lineLeft.get_line();
  if(readTop==LEFT||readTop==RIGHT){
    ret = false;
    if(readTop==LEFT){
      m_tL_speed-=M_SPEED;
      m_tR_speed-=M_SPEED;
    } else {
      m_tL_speed+=M_SPEED;
      m_tR_speed+=M_SPEED;
    }
  }
  if(readRight==LEFT||readRight==RIGHT){
    ret = false;
    if(readRight==LEFT){
      m_tR_speed-=M_SPEED;
      m_bR_speed-=M_SPEED;
    } else {
      m_tR_speed+=M_SPEED;
      m_bR_speed+=M_SPEED;
    }
  }
  if(readBottom==LEFT||readBottom==RIGHT){
    ret = false;
    if(readBottom==LEFT){
      m_bR_speed-=M_SPEED;
      m_bL_speed-=M_SPEED;
    } else {
      m_bR_speed+=M_SPEED;
      m_bL_speed+=M_SPEED;
    }
  }
  if(readLeft==LEFT||readLeft==RIGHT){
    ret = false;
    if(readLeft==LEFT){
      m_bL_speed-=M_SPEED;
      m_tL_speed-=M_SPEED;
    } else {
      m_bL_speed+=M_SPEED;
      m_tL_speed+=M_SPEED;
    }
  }
  updateSpeed(&m_topLeft,m_tL_speed);
  updateSpeed(&m_topRight,m_tR_speed);
  updateSpeed(&m_bottomLeft,m_bL_speed);
  updateSpeed(&m_bottomRight,m_bR_speed);
  //m_topLeft.runSpeed();
  //m_topRight.runSpeed();
  //m_bottomLeft.runSpeed();
  //m_bottomRight.runSpeed();
  //delay(5);
  return ret;
}  
  

void line_follow(){
  // white
  //red = 50; green = 50; blue = 50;
  //debug_colour();
  // use of pointers helps translate robot movement functions based on direction
  AccelStepper *m_frontLeft;
  AccelStepper *m_frontRight;
  AccelStepper *m_backLeft;
  AccelStepper *m_backRight;
  LineSensor *lineFront;
  LineSensor *lineBack;
  get_motor_alignment(&m_frontLeft,&m_frontRight,&m_backLeft,&m_backRight);
  get_line_alignment(&lineFront,&lineBack);
  line_pos_t readFront = lineFront->get_line();
  line_pos_t readBack = lineBack->get_line();
  if(readFront!=NONE&&readBack!=NONE){ // dual sensor control
    //front pair of motors
    if(readFront==RIGHT){
      updateSpeed(m_frontLeft,M_SPEED);
      updateSpeed(m_frontRight,-M_SLOW);
    } else if(readFront==LEFT){
      updateSpeed(m_frontLeft,M_SLOW);
      updateSpeed(m_frontRight,-M_SPEED);
    } else {
      updateSpeed(m_frontLeft,M_SPEED);
      updateSpeed(m_frontRight,-M_SPEED);
    }
    //back pair of motors
    if(readBack==RIGHT){
      updateSpeed(m_backLeft,M_SPEED);
      updateSpeed(m_backRight,-M_SLOW);
    } else if(readBack==LEFT){
      updateSpeed(m_backLeft,M_SLOW);
      updateSpeed(m_backRight,-M_SPEED);
    } else {
      updateSpeed(m_backLeft,M_SPEED);
      updateSpeed(m_backRight,-M_SPEED);
    }
  } else { // one sensor must shift whole robot
    if(readFront==RIGHT||readBack==LEFT){
      updateSpeed(m_frontLeft,M_SPEED);
      updateSpeed(m_backRight,-M_SPEED);
      updateSpeed(m_frontRight,-M_SLOW);
      updateSpeed(m_backLeft,M_SLOW);
    } else if(readFront==LEFT||readBack==RIGHT){
      updateSpeed(m_frontRight,-M_SPEED);
      updateSpeed(m_backLeft,M_SPEED);
      updateSpeed(m_frontLeft,M_SLOW);
      updateSpeed(m_backRight,-M_SLOW);
    } else {
      //lost the line
      updateSpeed(m_frontLeft,M_SPEED);
      updateSpeed(m_backLeft,M_SPEED);
      updateSpeed(m_frontRight,-M_SPEED);
      updateSpeed(m_backRight,-M_SPEED);
    }
  }
}

void get_line_alignment(LineSensor **l_F,LineSensor **l_B){
    switch(globalHeading){
    case 'u' :
      *l_F = &lineTop;
      *l_B = &lineBottom;
      break;
    case 'r' :
      *l_F = &lineRight;
      *l_B = &lineLeft;
      break;
    case 'd' :
      *l_F = &lineBottom;
      *l_B = &lineTop;
      break;
    case 'l' :
      *l_F = &lineLeft;
      *l_B = &lineRight;
      break;
    default : 
      *l_F = &lineTop;
      *l_B = &lineBottom;
      break;
  }
}

void get_motor_alignment(AccelStepper **m_fL,AccelStepper **m_fR,AccelStepper **m_bL,AccelStepper **m_bR){
  // global heading must have motor direction first
  switch(globalHeading){
    case 'u' :
      *m_fL = &m_topLeft;
      *m_fR = &m_topRight;
      *m_bL = &m_bottomLeft;
      *m_bR = &m_bottomRight;
      break;
    case 'r' :
      *m_fL = &m_topRight;
      *m_fR = &m_bottomRight;
      *m_bL = &m_topLeft;
      *m_bR = &m_bottomLeft;
      break;
    case 'd' :
      *m_fL = &m_bottomRight;
      *m_fR = &m_bottomLeft;
      *m_bL = &m_topRight;
      *m_bR = &m_topLeft;
      break;
    case 'l' :
      *m_fL = &m_bottomLeft;
      *m_fR = &m_topLeft;
      *m_bL = &m_bottomRight;
      *m_bR = &m_topRight;
      break;
    default : 
      *m_fL = &m_topLeft;
      *m_fR = &m_topRight;
      *m_bL = &m_bottomLeft;
      *m_bR = &m_bottomRight;
      break;
  }
}

void move_flag(){
  moveFlag = 1;
}

void move_robot() {
  if(currentHeading!=globalHeading){
    if(currentHeading=='0'){
      m_topLeft.enableOutputs();
      m_topRight.enableOutputs();
      m_bottomLeft.enableOutputs();
      m_bottomRight.enableOutputs();
    } else if(globalHeading=='0'){
      updateSpeed(&m_topLeft,0);
      updateSpeed(&m_topRight,0);
      updateSpeed(&m_bottomLeft,0);
      updateSpeed(&m_bottomRight,0);
      m_topLeft.disableOutputs();
      m_topRight.disableOutputs();
      m_bottomLeft.disableOutputs();
      m_bottomRight.disableOutputs();
    }
    currentHeading=globalHeading;
  }
  //line_follow();
  m_topLeft.runSpeed();
  m_topRight.runSpeed();
  m_bottomLeft.runSpeed();
  m_bottomRight.runSpeed();
}

void updateSpeed(AccelStepper *thisMotor,int newSpeed){
  int speedVal = (int)thisMotor->speed();
  if(speedVal!=newSpeed){
    thisMotor->setSpeed(newSpeed);
  }
}

void decide_direction(uint8_t options){  
  //Serial.print("Global heading: ");
  //Serial.println((char)globalHeading);
  //Serial.print("Passed in line options: ");
  //Serial.println(options,BIN);
  heading_t newHeading;
  heading_t directionList[4];
  uint8_t directionInts[4];
  float angle;
  // Make doubling back impossible. (maybe enable for pacman later)
  heading_t opposite_heading;
  switch(globalHeading) {
    case 'u': opposite_heading = 'd'; break;
    case 'd': opposite_heading = 'u'; break;
    case 'l': opposite_heading = 'r'; break;
    case 'r': opposite_heading = 'l'; break;
    default : opposite_heading = 0; break; 
  }
  options &= ~heading2binary(opposite_heading);
  //Serial.print("After removing opposite heading: ");
  //Serial.println(options,BIN);
  goal.x = 0;
  goal.y = 0;
  #ifdef USE_RADIO
  if (curr_command == MANUAL_OVERRIDE) {
    if (override_dir & options) {
    // If the override direction is available, set that heading
      globalHeading = binary2heading(override_dir);
    } else if (heading2binary(globalHeading) & options) {
      // If the current direction is available, maintain
      globalHeading = globalHeading; // lol
    } else {
      globalHeading = 0;
    }
    return;
  } 
  // This else is to
  #endif
  if(goal.x==0&&goal.y==0){ //random movement mode
    char randPriority[5] = "udlr";
    uint8_t tempDir;
    uint8_t randSelect;
    uint8_t valid  = 0;
    while(valid==0){
      randSelect = millis()%4;
      //Serial.print("rand select = ");
      //Serial.println(randSelect);
      
      newHeading = randPriority[randSelect];
      //Serial.println((char)newHeading);
      tempDir = heading2binary(newHeading);
      valid = ((tempDir&options)>0);
      //delay(200);
    }
    globalHeading = newHeading;
    return;  
//    newHeading = opposite_heading;
//    while (newHeading == opposite_heading) {
//       headingList[4]
//    }
//      // heading is going to be a single bit set
//     do {
//      uint8_t headingList[4] = {U,L,R,D};
//      uint8_t heading = options & headingList(random(0,4));
//      if (heading != 0) {
//        newHeading = binary2heading(heading);
//      }
//    } while (newHeading == 0);
//    globalHeading = newHeading;
//    return;
  }
  else if(thisRobot->p.y==goal.y&&thisRobot->p.x==goal.x){
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
  if (angle>=135) {
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
    //newHeading = '0';
    globalHeading = '0';
  } else {
    //newHeading = directionList[i];
    globalHeading = directionList[i];
  }  
}

/**    MAP NAVIGATION FUNCTIONS    **/
void set_goal(){
  switch(playerSelect){
    case PACMAN:
       break;
    case GHOST1:
      if(aiTime-millis()<15000){
        goal.x = 1;
        goal.y = 1;
      } else if(aiTime-millis()<35000){
        goal.x = game.pac.p.x;
        goal.y = game.pac.p.y;
      } else {
        aiTime = millis();
      }
      break;
    case GHOST2:
      if(aiTime-millis()<15000){
        goal.x = 9;
        goal.y = 9;
      } else if(aiTime-millis()<35000){
        goal.x = game.pac.p.x;
        goal.y = game.pac.p.y;
        int newX = goal.x;
        int newY = goal.y;
        calc_new_square(&newX,&newY,game.pac.h,5);
        map_constrain(&newX,&newY);
        goal.x = newX;
        goal.y = newY;
      } else {
        aiTime = millis();
      }
      break;
    case GHOST3:
      if(aiTime-millis()<15000){
        goal.x = 9;
        goal.y = 1;
      } else if(aiTime-millis()<35000){
        goal.x = random(1,9);
        goal.y = random(1,9);
      } else {
        aiTime = millis();
      }
      break;
    default:
      break;
  }
}


void map_constrain(int *x,int *y){
  *x = constrain(*x,1,9);
  *y = constrain(*y,1,9);
}

uint8_t* check_square(int x,int y){
  robot_t *r = NULL;
  if(x<1||x>9||y<1||y>9){
    r = NULL;
  } else if (game.pac.p.x==x&&game.pac.p.y==y){
    r = &game.pac;
  }  // probably shouldn't consider pacman's location
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
    
    // force look at the next intersection
    if(is_open(xDest,yDest,h)){
      calc_new_square(&xDest,&yDest,h,1);
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
  uint8_t options = 0b0000;
  int tempX, tempY;
  // expands along 
  if(is_open(x,y,'u')){
    tempX = x; tempY = y-1;
    r = expand(&tempX,&tempY,'u');
    if(r==NULL) {
      if(!collision_detect('u')){
        //Serial.println("Up path available");
        options|=U;
      }
    }
  }
  if(is_open(x,y,'r')){
    tempX = x+1; tempY = y;
    r = expand(&tempX,&tempY,'r');
    if(r==NULL) {
      if(!collision_detect('r')){
        //Serial.println("Right path available");
        options|=R;
      }
    }
  }
  if(is_open(x,y,'d')){
    tempX = x; tempY = y+1;
    r = expand(&tempX,&tempY,'d');
    if(r==NULL) {
      if(!collision_detect('d')){
        Serial.println("Down path available");
        options|=D;
      }
    }
  }
  if(is_open(x,y,'l')){
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
    if(is_open(*x,*y,h)){
      calc_new_square(x,y,h,1);
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
    if(is_open(*x,*y,h)){
      calc_new_square(x,y,h,1);
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
  if(r==(uint8_t*)&game.pac){
    r=NULL;
  }
  return r;
}

void calc_new_square(int *x,int*y,heading_t h,int d){
  uint8_t dir = heading2binary(h);
  *x = *x + ((xx&dir&addDir)>0)*d - ((xx&dir&subDir)>0)*d;
  *y = *y + ((yy&dir&addDir)>0)*d - ((yy&dir&subDir)>0)*d;
}
  
char binary2heading(uint8_t h) {
  char ret;
  switch(h){
    case 0b0001 : ret = 'u'; break;
    case 0b0010 : ret = 'r'; break;
    case 0b0100 : ret = 'd'; break;
    case 0b1000 : ret = 'l'; break;
    default  : ret = 0b0000; break;
  }
  return(ret);
  
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
