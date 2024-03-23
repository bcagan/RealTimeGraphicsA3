#include "platform.h"
#ifdef PLATFORM_LIN
#include "WindowManager_lin.h"
#include "iostream"
#include "Main.h"
#include "Mode.h"
#include <fstream>
#include "thread"
#include "MathHelpers.h"
#define RENDERDOC_HOOK_VULKAN 0

#include <X11/xlib.h>
#include <assert.h>
#include <unistd.h>
#define nullptr (0)

//https://learn.microsoft.com/en-us/windows/win32/learnwin32/creating-a-window

void freeMain() {
	CoUninitialize();
#ifndef  NDEBUG
	FreeConsole();
#endif //  DEBUG_CONSOLE
}

Button parseButtonKey(xcb_keycode_t detail) {
	std::cout << "key: " << detail << std::endl;
	Button button;
	return button;
}

Button parseButton(xcb_button_t detail) {
	std::cout << "button: " << detail << std::endl;
	Button button;
	return button;
}

void modeSpawnThread(ProgramMode* mode) {
	try
	{
		mode->modeMain();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return;
	}

	return;
}

int main(int argc, char* argv[])
{
	/*
	//Handle command line arguments
	int sceneArg = 0;
	int cameraArg = 0;
	int physicalDeviceArg = 0;
	int drawingSizeArgW = 0;
	int drawingSizeArgH = 0;
	int headlessArg = 0;
	int shaderArg = 0;
	int poolArg = 0;
	bool instancing = false;
	bool verbose = false;
	bool culling = false;
	bool animate = true;
	bool listPhysicalDevices = false;
	if (argc < 2) throw std::runtime_error("Please specify a scene (.s72 file) to load the program using --scene ____.");
	for (int arg = 0; arg < argc; arg++) {
		std::string isArg = argv[arg];
		if (
			arg < argc - 2 &&
			std::string(argv[arg + 1]).substr(0, 2).compare("--") &&
			std::string(argv[arg]).compare("--drawing-size") == 0) {
			drawingSizeArgW = arg + 1;
			drawingSizeArgH = arg + 2;
		}
		else if (arg < argc - 1 && std::string(argv[arg + 1]).substr(0, 2).compare("--")) {
			if (std::string(argv[arg]).compare("--scene") == 0) {
				sceneArg = arg + 1;
			}
			if (std::string(argv[arg]).compare("--shader") == 0) {
				shaderArg = arg + 1;
			}
			else if(std::string(argv[arg]).compare("--camera") == 0) {
				cameraArg = arg + 1;
			}
			else if (std::string(argv[arg]).compare("--physical-device") == 0) {
				physicalDeviceArg = arg + 1;
			}
			else if (std::string(argv[arg]).compare("--headless") == 0) {
				headlessArg = arg + 1;
			}
			else if (std::string(argv[arg]).compare("--pool-size") == 0) {
				poolArg = arg + 1;
			}
		}
		else if (std::string(argv[arg]).compare("--list-physical-devices") == 0) {
			listPhysicalDevices = true;
		}
		else if (std::string(argv[arg]).compare("--instancing") == 0) {
			instancing = true;
		}
		else if (std::string(argv[arg]).compare("--culling") == 0) {
			culling = true;
		}
		else if (std::string(argv[arg]).compare("--verbose") == 0) {
			verbose = true;
		}
		else if (std::string(argv[arg]).compare("--no-animate") == 0) {
			animate = false;
		}
		else if (std::string(argv[arg]).size() >= 2 &&
			std::string(argv[arg]).substr(0, 2).compare("--") == 0) {
			std::cout << "The following argument was incomplete: " << argv[arg] << std::endl;
		}
	}
	*/

	Mode graphMode;
	/*if (listPhysicalDevices) {
		graphMode.listPhysicalDevices();
		return 0;
	}
	std::string sceneFile;
	if (sceneArg == 0) {
		throw std::runtime_error("Please specify a scene (.s72 file) to load the program using --scene ____.");
	}
	else {
		sceneFile = std::string(argv[sceneArg]);
		size_t sceneLen = sceneFile.size();
		if (sceneLen < 4 ||
			std::string(argv[sceneArg]).substr(sceneLen - 4, 4).compare(".s72")) {
			throw std::runtime_error("Please specify a scene (.s72 file) to load the program using --scene ____.");
		}
	}
	//Camera: Optional
	std::string cameraName = "";
	if (cameraArg != 0) {
		cameraName = std::string(argv[cameraArg]);
	}
	//Physical device name: Required
	std::string physicalDeviceName = "";
	if (physicalDeviceArg == 0) {
		throw std::runtime_error("Please specify a valid phyiscal device name. To see those available, use --list-physical-devices");
	}
	else {
		physicalDeviceName = std::string(argv[physicalDeviceArg]);
	}
	//Drawing size: Optional
	int w = 1920; int h = 1080;
	if (drawingSizeArgW != 0 && drawingSizeArgH != 0) {
		w = atoi(argv[drawingSizeArgW]);
		h = atoi(argv[drawingSizeArgH]);
	}
	//Headless: Optional
	std::string headlessFile;
	if (headlessArg != 0 && (drawingSizeArgH || drawingSizeArgW)) {
		throw std::runtime_error("A specified drawing size is required to run in headless mode. Please do so using the argument --drawing-size.");
	}
	else {
		headlessFile = std::string(argv[headlessArg]);
	}
	//Shader path: required
	std::string shaderDirPath;
	if (shaderArg == 0) {
		throw std::runtime_error("Please specify a valid path to a shader directiory.");
	}
	else {
		shaderDirPath = std::string(argv[shaderArg]);
	}
	//Pool Arg: Optional
	int poolSize = 0;
	if (poolArg != 0) {
		poolSize = atoi(argv[poolArg]);
		graphMode.poolSize = poolSize;
	}
	//Instancing: optional
	graphMode.useInstancing = instancing;
	//Verbose: optional
	graphMode.verbose = verbose;
	//Culling: optional
	graphMode.culling = culling;
	//Animate: optional
	graphMode.animate = animate;
	*/

	//Open connection to X server
	xcb_connection_t* connection = xcb_connect(NULL, NULL);

	//Create windows and graph modes
	MainProgram mainProgram;
	graphMode.sceneName = sceneFile;
	graphMode.cameraName = cameraName;
	graphMode.deviceName = physicalDeviceName;
	graphMode.eventName = headlessFile;
	graphMode.shaderDir = shaderDirPath;
	mainProgram.setCurrentMode((ProgramMode*)(&graphMode));

	WindowManager windowManager;
	int mainWindowID = windowManager.createAndRegisterAWindowClass( w,h, connection, "Master");
	if (mainWindowID < 0) {
		std::cout << "Error when trying to create main window\n";
		return -1;
	}
	MasterWindow* mainWindow = windowManager.getWindowClass_Master(mainWindowID);
	graphMode.mainWindow = mainWindow;


	//Call any new threads here
	std::thread vulkanThread(modeSpawnThread, mainProgram.getCurrentMode());
	tagPOINT point;
	point.x = mainWindow->resolution.first / 2;
	point.y = mainWindow->resolution.second / 2;
	graphMode.defaultCursorPos = std::make_pair(point.x, point.y);
	//https://xcb.freedesktop.org/tutorial/events/
	//Handle main window messages
	xcb_generic_event_t* xcbEvent;
	while ((xcbEvent = xcb_wait_for_event(connection))) {
		InputEvent inputEvent;
		inputEvent.type = EVENT_NONE;
		switch (xcbEvent->response_type & ~0x80) {
		case XCB_EXPOSE: {
			break;
		}
		case XCB_BUTTON_PRESS: {
			inputEvent.type = BUTTON_EVENT;
			xcb_button_press_event_t * press = (xcb_button_press_event_t*)xcbEvent;
			inputEvent.input.button = parseButton(press->detail);
			inputEvent.input.button.x = press->event_x;
			inputEvent.input.button.y = press->event_y;
			inputEvent.input.button.pressed = true;
			inputEvent.input.button.doubleClick = false;
			break;
		}
		case XCB_BUTTON_RELEASE:
		{
			xcb_button_release_event_t * release = (xcb_button_release_event_t*)xcbEvent;
			inputEvent.type = BUTTON_EVENT;
			inputEvent.input.button = parseButton(release->detail);
			inputEvent.input.button.x = release->event_x;
			inputEvent.input.button.y = release->event_y;
			inputEvent.input.button.pressed = true;
			inputEvent.input.button.doubleClick = false;
			break;
		}
		case XCB_MOTION_NOTIFY: {
			xcb_motion_notify_event_t* motion = (xcb_motion_notify_event_t*)xcbEvent;
			inputEvent.type = MOUSE_MOVE;
			inputEvent.input.move.x = motion->event_x;
			inputEvent.input.move.y = motion->event_y;
			break;
		}
		case XCB_ENTER_NOTIFY: {
			break;
		}
		case XCB_LEAVE_NOTIFY: {
			break;
		}
		case XCB_KEY_PRESS: {
			inputEvent.type = BUTTON_EVENT;
			xcb_key_press_event_t* keyPress = (xcb_key_press_event_t*)xcbEvent;
			inputEvent.input.button = parseButtonKey(release->detail);
			inputEvent.input.button.pressed = true;
			inputEvent.input.button.doubleClick = false;
		}
		case XCB_KEY_RELEASE: {
			inputEvent.type = BUTTON_EVENT;
			xcb_key_release_event_t* keyRelease = (xcb_key_release_event_t*)xcbEvent;
			inputEvent.input.button = parseButtonKey(release->detail);
			inputEvent.input.button.pressed = false;
			inputEvent.input.button.doubleClick = false;

		}
		default{
			break;
		}
		}
		master->sendEventToProgram(inputEvent);
	}
	mainProgram.getCurrentMode()->active = false;
	vulkanThread.join();
	freeMain();
	xcb_disconnect(connection);
	return 0;
}

ProgramMode* MainProgram::getCurrentMode() {

	return this->currentMode;
}
#endif