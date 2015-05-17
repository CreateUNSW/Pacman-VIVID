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

//-----------------------------------------------------------------------------------------------------
//Define
//-----------------------------------------------------------------------------------------------------


//-----------------------------------------------------------------------------------------------------
//Constants
//-----------------------------------------------------------------------------------------------------

//Pins
const int confirmPin = ;	//Pin to give acknowledge that it has received the command
const int gameBegin = ;		//Input pin to know when game has begun
const int gameEnd = ;		//input pin to know when pacman dies

//-----------------------------------------------------------------------------------------------------
//Variables
//-----------------------------------------------------------------------------------------------------

//Variables


//Functions


//-----------------------------------------------------------------------------------------------------
//Main
//-----------------------------------------------------------------------------------------------------

void setup(){
	
	//Set Output Pins
	pinMode(confirmPin, OUTPUT);
	
	//Set Input Pins
	pinMode(gameBegin, INPUT);
	pinMode(gameEnd, INPUT);
	
	player.begin();	//Initialises the music Player
	
}

void loop(){
	
}
//-----------------------------------------------------------------------------------------------------
//Subroutines
//-----------------------------------------------------------------------------------------------------