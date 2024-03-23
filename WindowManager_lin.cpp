#include "platform.h"
#ifdef PLATFOMR_LIN
#include "WindowManager_lin.h"
#include "string"
#include <map>
#include <vector>
#include "iostream"
#include <map>
#include "Main.h"


//Creates windows and puts window in dictionary for 
//(currently unused) multi-window support
//https://tronche.com/gui/x/xlib-tutorial/2nd-program-anatomy.html
int WindowManager::createAndRegisterAWindowClass(int w, int h, 
	xcb_connection_t* connection, std::string className) {
	if (std::string(className).compare("Master") == 0) {
		MasterWindow newWindow;
		windowDict_Master.insert(std::make_pair(idCount,newWindow));
		auto newWindowIterator = windowDict_Master.find(idCount);
		if (newWindowIterator == windowDict_Master.end()) return -1;
		if (newWindowIterator->second.createWindowClass(connection, 0,0,w,h) == 0) return -1;
		idCount++;
		return idCount - 1;
	}
	else return -1;
}

std::string MasterWindow::className() { return "Master"; }



MasterWindow* WindowManager::getWindowClass_Master(int classId) {
	std::map<int, MasterWindow>::iterator windowClass = windowDict_Master.find(classId);
	if (windowClass == windowDict_Master.end()) {
		throw - 1;
	}
	else return &windowClass->second;
}


#endif