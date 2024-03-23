#pragma once
#include <vector>
#include "iostream"
#include <map>
#include <array>
#include "string"
#include "Main.h"
#include <iostream>
#include "platform.h"
#ifdef PLATFOMR_LIN

#include <X11/xlib.h>
#include <assert.h>
#include <unistd.h>

template<class DERIVED_TYPE>
class WindowLin;
template<class DERIVED_TYPE> using
WindowMainPair = std::pair<WindowLin<DERIVED_TYPE>*, MainProgram*>;

//Generic window class
//https://xcb.freedesktop.org/tutorial/basicwindowsanddrawing/
template<class DERIVED_TYPE>
class WindowLin {
public:
	std::pair<int, int> resolution;
	std::pair<int, int> pos;

	//Unused but allows cursore to be reset platform agnostically by program
	void updateCursorPos(std::pair<int, int> newPos) {
		SetCursorPos(newPos.first, newPos.second);
	}
	
	//Create actual window
	int createWindowClass(xcb_connection_t* con, int x, int y, int w, int h) {
		connection = con;
		const xcb_setup_t* setup = xcb_get_setup(connection);
		xcb_screen_iterator_t iterator = xcb_setup_roots_iterator(setup);
		xcb_screen_t* screen = iter.data;

		//Create window
		window = xcb_generate_id(connection);
		uint32_t mask = XCB_CW_EVENT_MASK | XCB_CW_CURSOR;
		uint32_t valueWin[] = { XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,
			XCB_EVENT_MASK_EXPOSURE };
		xcb_create_window(connection, XCB_COPY_FROM_PARENT, window,
			screen->root, x, y, w, h, 10, XCB_WINDOW_CLASS_INPUT_OUTPUT,
			screen->root_visual, mask, valueWin);
		xcb_map_window(connection, window);
		xcb_flush(connection);

		//Create graphics context with black background
		gc = xcb_generate_id(connection);
		mask = XCB_GC_BACKGROUND;
		uint32_t valueGC[] = { screen->black_pixel };
		xcb_create_gc(connection, black, window, mask, valueGC);
		xcb_flush(connection);
	}

	//TODO: Linux equiv HWND getHwnd() { return localHwnd; }

	void show(int nCmdShow) {
		ShowWindow(this->localHwnd, nCmdShow);
	}

	xcb_connection_t* getConnection() {
		return connection;
	}

	xcb_window_t getWindow() {
		return window;
	}


protected:
	uint32_t id;

	xcb_connection_t* connection;
	xcb_window_t window;
	xcb_gcontext_t gc;
};

class MasterWindow : public WindowLin<MasterWindow> {
public:
	std::string className();
};

//Class to manage and store multiple windows
class WindowManager
{
public:
	int createAndRegisterAWindowClass();
	MasterWindow* getWindowClass_Master(int classID);
private:
	//Window arrays
	int idCount = 0;
	std::map<int, MasterWindow> windowDict_Master = std::map<int, MasterWindow>(int w, int h, 
		xcb_connection_t* connection, std::string className);

	
};



#endif