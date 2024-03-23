#pragma once
#include "Windows.h"
#include <vector>
#include "iostream"
#include <map>
#include <array>
#include "string"
#include "Main.h"
#include <iostream>

template<class DERIVED_TYPE>
class Window;
template<class DERIVED_TYPE> using
WindowMainPair = std::pair<Window<DERIVED_TYPE>*, MainProgram*>;

//Generic window class
template<class DERIVED_TYPE>
class Window {
public:
	Window() : localHwnd(NULL), id(0) { };

	std::pair<int, int> resolution;
	std::pair<int, int> pos;

	//Unused but allows cursore to be reset platform agnostically by program
	void updateCursorPos(std::pair<int, int> newPos) {
		SetCursorPos(newPos.first, newPos.second);
	}
	
	//Create actual window
	int createWindowClass(HINSTANCE hInstance,
		uint32_t idInput,
		DWORD dwStyle,
		const wchar_t windowText[],
		MainProgram* masterP,
		DWORD dwExStyle = 0,
		int x = CW_USEDEFAULT, 
		int y = CW_USEDEFAULT, 
		int width = CW_USEDEFAULT, 
		int height = CW_USEDEFAULT) {

		//Create new windows class
		WNDCLASS windowClass = { 0 };
		windowClass.lpfnWndProc = WindowProc;
		windowClass.hInstance = hInstance;
		windowClass.lpszClassName = className();
		windowClass.style = CS_DBLCLKS;
		WindowMainPair<DERIVED_TYPE>* windowMasterPair = new WindowMainPair<DERIVED_TYPE>();
		windowMasterPair->first = this;
		windowMasterPair->second = masterP;

		//Register class in windows and in local manager
		RegisterClass(&windowClass);

		localHwnd = CreateWindowEx(
			dwExStyle,
			className(), 
			windowText,
			dwStyle,

			x, y, width, height,

			NULL,
			NULL,
			hInstance,
			windowMasterPair
		);
		std::cout << (localHwnd != NULL) << std::endl;
		if (localHwnd == NULL) return 0;
		id = idInput;
		resolution = std::make_pair(width, height);
		pos = std::make_pair(x, y);
		return 1;
	}

	HWND getHwnd() { return localHwnd; }

	void show(int nCmdShow) {
		ShowWindow(this->localHwnd, nCmdShow);
	}

	//Either initialize message handler, or load handler given a window event
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		WindowMainPair<DERIVED_TYPE>* thisPointer = NULL;

		if (uMsg == WM_NCCREATE) {
			WindowMainPair<DERIVED_TYPE>* windowMasterPair = (WindowMainPair<DERIVED_TYPE>*)(((CREATESTRUCT*)lParam)->lpCreateParams);
			thisPointer = windowMasterPair;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)windowMasterPair);
			thisPointer->first->localHwnd = hwnd;
		}
		else {
			thisPointer = (WindowMainPair<DERIVED_TYPE>*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		}

		if (thisPointer != NULL && thisPointer->first != NULL && thisPointer->second != NULL) {
			return thisPointer->first->handleMessage(uMsg, hwnd, wParam, lParam, thisPointer->second);
		}
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}


protected:
	virtual LRESULT handleMessage(UINT uMsg, HWND hwnd, WPARAM wParam, LPARAM lParam, MainProgram* master) = 0;
	virtual LPCWSTR  className() = 0;
	HWND localHwnd;
	uint32_t id;
};

class MasterWindow : public Window<MasterWindow> {
public:
	LRESULT handleMessage(UINT uMsg, HWND hwnd, WPARAM wParam, LPARAM lParam, MainProgram* master);
	LPCWSTR  className();
};

//Class to manage and store multiple windows
class WindowManager
{
public:
	int createAndRegisterAWindowClass(HINSTANCE hInstance, int nCmdShow, const wchar_t className[], const wchar_t windowText[], bool showOnCreation, MainProgram* master, int w, int h);
	MasterWindow* getWindowClass_Master(int classID);
private:
	//Window arrays
	int idCount = 0;
	std::map<int, MasterWindow> windowDict_Master = std::map<int, MasterWindow>();

	
};



