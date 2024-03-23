#include "Windows.h"
#include "WindowManager_win.h"
#include "iostream"
#include "Main.h"
#include "Mode.h"
#include <fstream>
#include "thread"
#include "MathHelpers.h"

#define RENDERDOC_HOOK_VULKAN 0

//https://learn.microsoft.com/en-us/windows/win32/learnwin32/creating-a-window

void freeMain() {
	CoUninitialize();
#ifndef  NDEBUG
	FreeConsole();
#endif //  DEBUG_CONSOLE
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
#ifndef  NDEBUG
	//Create command line in addition to main menu
	//https://stackoverflow.com/questions/2561399/subsystemwindows-program-will-not-write-to-command-line
	AllocConsole();
	std::ofstream conout("con");
	std::wofstream conoutw("con");
	std::cout.rdbuf(conout.rdbuf());
	std::cerr.rdbuf(conout.rdbuf());
	std::wcout.rdbuf(conoutw.rdbuf());
#endif


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

	Mode graphMode;
	if (listPhysicalDevices) {
		graphMode.listPhysicalDevices();
		return 0;
	}
	//Scene: REQUIRED
	//CURRENTLY OPTIONAL UNTIL A2 SCENE EXAMPLES ARE AVAIALBLE
	//FOR NOW, DO NOT PASS IN SCENE!!! MANUALLY CREATE TEST GRAPHS
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

	//Create windows and graph modes
	HRESULT hResult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	MainProgram mainProgram;
	graphMode.sceneName = sceneFile;
	graphMode.cameraName = cameraName;
	graphMode.deviceName = physicalDeviceName;
	graphMode.eventName = headlessFile;
	graphMode.shaderDir = shaderDirPath;
	mainProgram.setCurrentMode((ProgramMode*)(&graphMode));

	WindowManager windowManager;
	auto hInstance = GetModuleHandle(NULL);
	int mainWindowID = windowManager.createAndRegisterAWindowClass(hInstance, true, L"Master", L"My First Windows", true, &mainProgram, w,h);


	if (mainWindowID < 0) {
		std::cout << "Error when trying to create main window\n";
		return -1;
	}
	MasterWindow* mainWindow = windowManager.getWindowClass_Master(mainWindowID);
	graphMode.mainWindow = mainWindow;

	std::wcout << "Succesfully built window of class " <<
		windowManager.getWindowClass_Master(mainWindowID)->className() << std::endl;

	ShowCursor(true);

	//Call any new threads here
	std::thread vulkanThread(modeSpawnThread, mainProgram.getCurrentMode());
	tagPOINT point;
	point.x = mainWindow->resolution.first / 2;
	point.y = mainWindow->resolution.second / 2;
	graphMode.defaultCursorPos = std::make_pair(point.x, point.y);
	//Handle main window messages
	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	mainProgram.getCurrentMode()->active = false;
	vulkanThread.join();
	freeMain();
	return 0;
}

ProgramMode* MainProgram::getCurrentMode() {

	return this->currentMode;
}
