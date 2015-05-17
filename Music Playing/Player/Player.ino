/* 
  Description:
  Music player program for pacman vivid.
  Most likely we are going to use the music shield from
  Arduinos in Australia.
  We are also going to use the already pre-written library
  to play the Pacman music.
  
  Author:
  Christopher Chun-Hung Ho
  
  History:
  v1.0  17/4/15 initial code
*/

//-----------------------------------------------------------------------------------------------------
//Include
//-----------------------------------------------------------------------------------------------------

#include <SD.h>
#include <SPI.h>
#include <arduino.h>
#include <MusicPlayer.h>

#include <SoftwareSerial.h>

//-----------------------------------------------------------------------------------------------------
//Define
//-----------------------------------------------------------------------------------------------------
#define SIGNAL 11

//-----------------------------------------------------------------------------------------------------
//Constants
//-----------------------------------------------------------------------------------------------------

//Pins
const int waiter = 9;

//-----------------------------------------------------------------------------------------------------
//Variables
//-----------------------------------------------------------------------------------------------------

//Variables
byte track;

//Functions
SoftwareSerial matlab(11,12);
MusicPlayer singlePlayer;
MusicPlayer wakaPlayer;

void emptyPlayer();

//-----------------------------------------------------------------------------------------------------
//Main
//-----------------------------------------------------------------------------------------------------

void setup(){
  
  //set up Serial
  Serial.begin(9600);  //Initialises serial port
  matlab.begin(9600);  //Initialises serial comm with matlab
  
  //Set up output pins
  pinMode(SIGNAL, OUTPUT);  //set up the output pin
  
  //Set up input pins
  pinMode(waiter, INPUT);
  
  singlePlayer.begin();  //initialises the single playing playlist.
  wakaPlayer.begin();	 //Initialises the music Player
  
  singlePlayer.setPlayMode(PM_NORMAL_PLAY);
  wakaPlayer.setPlayMode(PM_REPEAT_ONE);
  
  wakaPlayer.addToPlaylist("Chomp.wav");
}

void loop(){
  
  while(!matlab.available()){    //Wait until Serial command to receive data
  }
  switch(matlab.read()){
    case 1:
      emptyPlaylist();
      singlePlayer.addToPlaylist("Begin.wav");
      singlePlayer.opPlay();
      //wait until song finishes
      singlePlayer.opStop();
      wakaPlayer.opPlay();
      break;
    case 2:                                            //play victory song
      waka.Player.opStop();
      emptyPlaylist();
      singlePlayer.addToPlaylist("Inter.wav");
      singlePlayer.opPlay();
      //wait until song finishes
      break;
    case 3:
      wakaPlayer.opStop();
      emptyPlaylist();
      singlePlayer.addToPlaylist("Death.wav");
      singlePlayer.opPlay();
      //wait until song finishes
      break;
    default:
      break;
  }
  
}
//-----------------------------------------------------------------------------------------------------
//Subroutines
//-----------------------------------------------------------------------------------------------------

void emptyPlaylist(){
  singlePlayer.deleteSong("Begin.wav");
  singlePlayer.deleteSong("Death.wav");
  singlePlayer.deleteSong("Inter.wav");
}
