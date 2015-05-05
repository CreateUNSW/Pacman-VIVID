#include "VividStepper.h"

VividStepper::VividStepper(VividStepper::stepType_t mode,uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4){
	_pins[0] = pin1; _pins[1] = pin2; _pins[2] = pin3; _pins[3] = pin4;
	uint8_t i;
	for (i = 0; i < 4; i++){
		pinMode(_pins[i], OUTPUT);
	}
	_currentDir = CW;
	_stepType = mode;
	_stepNumber = 0;
}

void VividStepper::step(stepperDir_t dir){
	if (dir == CW){
		if (_stepNumber + _stepType >= 8){
			_stepNumber = _stepType - 1;
		} else {
			_stepNumber+=_stepType;
		}
	} else if (dir = CCW){
		if (_stepNumber - _stepType < 0){
			_stepNumber = 7;
		} else {
			_stepNumber-=_stepType;
		}
	}
	uint8_t outputs = 0b000;
	switch (_stepNumber){
	case 0:
		outputs = 0b0001;
		break;
	case 1:
		outputs = 0b0101;
		break;
	case 2:
		outputs = 0b0100;
		break;
	case 3:
		outputs = 0b0110;
		break;
	case 4:
		outputs = 0b0010;
		break;
	case 5:
		outputs = 0b1010;
		break;
	case 6:
		outputs = 0b1000;
		break;
	case 7:
		outputs = 0b1001;
		break;
	}
	uint8_t i;
	for (i = 0; i < 4; i++){
		digitalWrite(_pins[i], (outputs & (1 << i)));
	}
	
}

void VividStepper::off(){
	uint8_t i;
	for (i = 0; i < 4; i++){
		digitalWrite(_pins[i], LOW);
	}
}