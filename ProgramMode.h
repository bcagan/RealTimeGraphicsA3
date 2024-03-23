#pragma once
#include "Windows.h"
#include "iostream"
#include "string"

//Platform agnostic types for window events
enum Button {
	LEFTBUTTON,
	RIGHTBUTTON,
	MIDDLEBUTTON,
	SIDEBUTTON,
	KEY_W,
	KEY_A,
	KEY_S,
	KEY_D,
	KEY_Q,
	KEY_E,
	KEY_0,
	KEY_1,
	KEY_2,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_DOWN,
	KEY_UP,
	KEY_NONE
};

struct ButtonEvent {
	Button button;
	bool pressed;
	bool doubleClick;
	int x; //For mouse move reset
	int y;
};

enum EventType {
	MOUSE_MOVE,
	BUTTON_EVENT,
	MOUSE_SCROLL,
	CHAR_EVENT,
	EVENT_NONE
};

struct MouseMove {
	int x;
	int y;
};

union EventInput {
	ButtonEvent button;
	int scrollDelta;
	MouseMove move;
	wchar_t charEvent;
};

struct InputEvent {
	EventType type;
	EventInput input;
};


//Generic mode class
class ProgramMode
{

public:
	virtual void handleinputEvent(InputEvent inputEvent) = 0;
	virtual int modeMain() = 0;
	bool active = true;
};

