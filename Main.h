#pragma once
#include "ProgramMode.h"
#include "iostream"
#include "Windows.h"

//Create a Master class which handles current mode, program state, etc.
//Make this available to the windows, so that they can access this function
//Make these functions a part of Master, and in turn, make the call the corresponding
//gmae mode function to pass input into game



class MainProgram {
public:

	void setCurrentMode(ProgramMode* mode) {
		currentMode = mode;
	}

	ProgramMode* getCurrentMode();

	void sendEventToProgram(InputEvent inputEvent) {
		currentMode->handleinputEvent(inputEvent);
	}


private:

	ProgramMode* currentMode;
};