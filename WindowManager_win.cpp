#include "WindowManager_win.h"
#include "Windows.h"
#include "windowsx.h"
#include "string"
#include <map>
#include <vector>
#include "iostream"
#include <map>
#include "Main.h"


//Creates windows and puts window in dictionary for 
//(currently unused) multi-window support
//https://learn.microsoft.com/en-us/windows/win32/learnwin32/creating-a-window
int WindowManager::createAndRegisterAWindowClass(HINSTANCE hInstance, int nCmdShow, const wchar_t className[], const wchar_t windowText[], bool showOnCreation, MainProgram* master, int w, int h) {
	if (std::wstring(className).compare(L"Master") == 0) {
		MasterWindow newWindow;
		windowDict_Master.insert(std::make_pair(idCount,newWindow));
		auto newWindowIterator = windowDict_Master.find(idCount);
		if (newWindowIterator == windowDict_Master.end()) return -1;
		if (newWindowIterator->second.createWindowClass(hInstance, idCount, WS_OVERLAPPEDWINDOW, windowText, master, 0, CW_USEDEFAULT, CW_USEDEFAULT, w, h) == 0) return -1;
		if (showOnCreation) newWindowIterator->second.show(nCmdShow);
		idCount++;
		return idCount - 1;
	}
	else return -1;
}

//Convert from windows virtual key codes to window event button enum
Button processVKey(WPARAM wParam) {
	switch (wParam) {
	case 0x41:
		return KEY_A;
	case 0x44:
		return KEY_D;
	case 0x45:
		return KEY_E;
	case 0x51:
		return KEY_Q;
	case 0x53:
		return KEY_S;
	case 0x57:
		return KEY_W;
	case VK_LEFT:
		return KEY_LEFT;
	case VK_RIGHT:
		return KEY_RIGHT;
	case VK_UP:
		return KEY_UP;
	case VK_DOWN:
		return KEY_DOWN;
	case 0x30:
		return KEY_0;
	case 0x31:
		return KEY_1;
	case 0x32:
		return KEY_2;
	default:
		return KEY_NONE;
	}
}

//Handles and parses all windows messages and converts them into 
//generic window events to be used in rest of program
LRESULT MasterWindow::handleMessage(UINT uMsg, HWND hwnd, WPARAM wParam, LPARAM lParam, MainProgram* master)
{	
	int x = 0; int y = 0;
	InputEvent inputEvent;
	inputEvent.type = EVENT_NONE;
	if (!localHwnd) {
		return  TRUE;
	}
	switch (uMsg) {
	case WM_DESTROY:
	{
		PostQuitMessage(0);
	}
	return 0;
	case WM_MOVE: {
		x = LOWORD(lParam);
		y = HIWORD(lParam);
		pos = std::make_pair(x, y);	
		return DefWindowProc(localHwnd, uMsg, wParam, lParam);
	}
	case WM_SIZE: //Internally handle standard window resize
	{
		x = LOWORD(lParam);
		y = HIWORD(lParam);
		resolution = std::make_pair(x, y);
	}
	//User input messages
	//Mouse events
	case WM_LBUTTONDOWN:
		inputEvent.type = BUTTON_EVENT;
		inputEvent.input.button.button = LEFTBUTTON;
		inputEvent.input.button.pressed = true;
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		inputEvent.input.button.x = x;
		inputEvent.input.button.y = y;
		break;
	case WM_LBUTTONUP:
		inputEvent.type = BUTTON_EVENT;
		inputEvent.input.button.button = LEFTBUTTON;
		inputEvent.input.button.pressed = false;
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		inputEvent.input.button.x = x;
		inputEvent.input.button.y = y;
		break;
	case WM_LBUTTONDBLCLK:
		inputEvent.type = BUTTON_EVENT;
		inputEvent.input.button.button = LEFTBUTTON;
		inputEvent.input.button.pressed = true;
		inputEvent.input.button.doubleClick = true;
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		inputEvent.input.button.x = x;
		inputEvent.input.button.y = y;
		break;
	case WM_RBUTTONDOWN:
		inputEvent.type = BUTTON_EVENT;
		inputEvent.input.button.button = RIGHTBUTTON;
		inputEvent.input.button.pressed = true;
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		inputEvent.input.button.x = x;
		inputEvent.input.button.y = y;
		break;
	case WM_RBUTTONUP:
		inputEvent.type = BUTTON_EVENT;
		inputEvent.input.button.button = RIGHTBUTTON;
		inputEvent.input.button.pressed = false;
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		inputEvent.input.button.x = x;
		inputEvent.input.button.y = y;
		break;
	case WM_RBUTTONDBLCLK:
		inputEvent.type = BUTTON_EVENT;
		inputEvent.input.button.button = RIGHTBUTTON;
		inputEvent.input.button.pressed = true;
		inputEvent.input.button.doubleClick = true;
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		inputEvent.input.button.x = x;
		inputEvent.input.button.y = y;
		break;
	case WM_MBUTTONDOWN:
		inputEvent.type = BUTTON_EVENT;
		inputEvent.input.button.button = MIDDLEBUTTON;
		inputEvent.input.button.pressed = true;
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		inputEvent.input.button.x = x;
		inputEvent.input.button.y = y;
		break;
	case WM_MBUTTONUP:
		inputEvent.type = BUTTON_EVENT;
		inputEvent.input.button.button = MIDDLEBUTTON;
		inputEvent.input.button.pressed = false;
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		inputEvent.input.button.x = x;
		inputEvent.input.button.y = y;
		break;
	case WM_MBUTTONDBLCLK:
		inputEvent.type = BUTTON_EVENT;
		inputEvent.input.button.button = MIDDLEBUTTON;
		inputEvent.input.button.pressed = true;
		inputEvent.input.button.doubleClick = true;
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		inputEvent.input.button.x = x;
		inputEvent.input.button.y = y;
		break;
	case WM_XBUTTONDOWN:
		inputEvent.type = BUTTON_EVENT;
		inputEvent.input.button.button = SIDEBUTTON;
		inputEvent.input.button.pressed = true;
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		inputEvent.input.button.x = x;
		inputEvent.input.button.y = y;
		break;
	case WM_XBUTTONUP:
		inputEvent.type = BUTTON_EVENT;
		inputEvent.input.button.button = SIDEBUTTON;
		inputEvent.input.button.pressed = false;
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		inputEvent.input.button.x = x;
		inputEvent.input.button.y = y;
		break;
	case WM_XBUTTONDBLCLK:
		inputEvent.type = BUTTON_EVENT;
		inputEvent.input.button.button = SIDEBUTTON;
		inputEvent.input.button.pressed = true;
		inputEvent.input.button.doubleClick = true;
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		inputEvent.input.button.x = x;
		inputEvent.input.button.y = y;
		break;
	case WM_MOUSEMOVE:
	{
		x = GET_X_LPARAM(lParam);
		y = GET_Y_LPARAM(lParam);
		inputEvent.type = MOUSE_MOVE;
		inputEvent.input.move.x = x;
		inputEvent.input.move.y = y;
		break;
	}
	
	//Mouse wheel
	case WM_MOUSEWHEEL:
	{
		int delta = GET_WHEEL_DELTA_WPARAM(wParam);
		inputEvent.type = MOUSE_SCROLL;
		inputEvent.input.scrollDelta = delta;
		break;
	}
	//Keyboard
	case WM_KEYUP:
	{
		inputEvent.type = BUTTON_EVENT;
		inputEvent.input.button.pressed = false;
		inputEvent.input.button.button = processVKey(wParam);
		break;
	}
	case WM_KEYDOWN:
	{
		inputEvent.type = BUTTON_EVENT;
		inputEvent.input.button.pressed = true;
		inputEvent.input.button.button = processVKey(wParam);
		break;
	}
	case WM_CHAR:
	{
		inputEvent.type = CHAR_EVENT;
		inputEvent.input.charEvent = (wchar_t)wParam;
		break;
	}
	default:
		return DefWindowProc(localHwnd, uMsg, wParam, lParam);
	}

	master->sendEventToProgram(inputEvent);
	return TRUE;
}

LPCWSTR MasterWindow::className() { return L"Master"; }



MasterWindow* WindowManager::getWindowClass_Master(int classId) {
	std::map<int, MasterWindow>::iterator windowClass = windowDict_Master.find(classId);
	if (windowClass == windowDict_Master.end()) {
		throw - 1;
	}
	else return &windowClass->second;
}


