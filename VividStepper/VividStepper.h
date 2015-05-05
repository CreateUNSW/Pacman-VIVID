#ifndef VividStepper_h
#define VividStepper_h

#include <stdlib.h>
#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#include <wiring.h>
#endif

typedef enum { CW, CCW } stepperDir_t;

class VividStepper
{
public:
	VividStepper();
	typedef enum { HALF=1, FULL=2 } stepType_t;
	VividStepper(VividStepper::stepType_t mode, uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4);
	void step(stepperDir_t dir);
	void off();
private:
	uint8_t _pins[4];
	stepperDir_t _currentDir;
	stepType_t _stepType;
	uint8_t _stepNumber;
};

#endif
