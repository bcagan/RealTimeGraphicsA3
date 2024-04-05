#include "Mode.h"
#include "ProgramMode.h"
#include "SceneGraph.h"
#include "VulkanSystem.h"
#include "Parser.h"
#include "Events.h"

//Handle each distinct type of window event
//Track delta over a frame, and change in x and y over a frame, and act on that
void Mode::handleMouseMove(int x, int y) {
	if(mouseEventActive){ //Only handle mouse movement while mouse is being moved
		std::pair<int, int> newDelta = std::make_pair(x - lastMousePos.first, y - lastMousePos.second);
		if (newDelta.first == 0 && newDelta.second == 0) return;
		moveStatus.mouseDelta.first += newDelta.first;
		moveStatus.mouseDelta.second += newDelta.second;
		lastMousePos = std::make_pair(x, y);

	}
}
//Scrolling unused
void Mode::handleScrollEvent(int delta) {
}

//Handle keyboard input
void Mode::handleButtonEvent(ButtonEvent event) {
	//Move
	if (event.button == KEY_W) { 
		moveStatus.forward = event.pressed;
	}
	if (event.button == KEY_A) { 
		moveStatus.left = event.pressed;
	}
	if (event.button == KEY_S) { 
		moveStatus.backward = event.pressed;
	}
	if (event.button == KEY_D) { 
		moveStatus.right = event.pressed;
	}
	if (event.button == KEY_Q) {
		moveStatus.up = event.pressed;
	}
	if (event.button == KEY_E) {
		moveStatus.down = event.pressed;
	}
	//Change camera mode
	if (event.button == KEY_0 && event.pressed) {
		vulkanSystem.movementMode = MOVE_DEBUG;
	}
	if (event.button == KEY_1 && event.pressed) {
		vulkanSystem.movementMode = MOVE_USER;
	}
	if (event.button == KEY_2 && event.pressed) {
		vulkanSystem.movementMode = MOVE_STATIC;
	}
	//Control animation
	if (event.button == KEY_LEFT && event.pressed) {
		vulkanSystem.forwardAnimation = false;
	}
	if (event.button == KEY_RIGHT && event.pressed) {
		vulkanSystem.forwardAnimation = true;
	}
	if (event.button == KEY_DOWN && event.pressed) {
		vulkanSystem.playingAnimation = false;
	}
	if (event.button == KEY_UP && event.pressed) {
		vulkanSystem.playingAnimation = true;
	}

	//Control camera via left click
	if (event.button == LEFTBUTTON && event.pressed) {
		mouseEventActive = true;
		lastMousePos = std::make_pair(event.x, event.y);
		ShowCursor(false);
	}
	else if (event.button == LEFTBUTTON && !event.pressed) {
		mouseEventActive = false;
		ShowCursor(true);
	}
}
//Char input unused
void Mode::handleCharEvent(wchar_t inChar) {

}

//Call respective input event
void Mode::handleinputEvent(InputEvent inputEvent) {
	switch (inputEvent.type) {
	case BUTTON_EVENT:
		handleButtonEvent(inputEvent.input.button);
		return;
	case MOUSE_MOVE:
		handleMouseMove(inputEvent.input.move.x, inputEvent.input.move.y);
		return;
	case MOUSE_SCROLL:
		handleScrollEvent(inputEvent.input.scrollDelta);
		return;
	case CHAR_EVENT:
		handleCharEvent(inputEvent.input.charEvent);
		return;
	default:
		return;
	}
}


//Main render loop
//Camera implementation guided by https://learnopengl.com/Getting-started/Camera
void Mode::mainLoop(SceneGraph* graph) {
	int framecount = 0;
	float mscount = 0;
	vulkanSystem.activeP = &active;
	float yaw = 0; float pitch = 0;
	float yawDebug = 0; float pitchDebug = 0;
	while (active) {
		//Process user input for the current frame
		float delta = 0;
		std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
		while (delta < 1 / 60.f) {
			now = std::chrono::high_resolution_clock::now();
			delta = std::chrono::duration<float>(now - lastFrame).count();
		}
		lastFrame = now;
		if (vulkanSystem.renderToWindow) {
			float_3 moveVec = vulkanSystem.movementMode == MOVE_DEBUG ? vulkanSystem.debugMoveVec : vulkanSystem.moveVec;
			if (vulkanSystem.movementMode != MOVE_STATIC) {
				float x = (moveStatus.left * 1 + moveStatus.right * -1) * delta * speed;
				float z = (moveStatus.forward * 1 + moveStatus.backward * -1) * delta * speed;
				float y = (moveStatus.up * 1 + moveStatus.down * -1) * delta * speed;
				moveVec = moveVec + float_3(x, y, z);
			}

			float_3 dirVec;
			if (vulkanSystem.movementMode == MOVE_DEBUG) {
				float moveFactor = 10;
				yawDebug += delta * moveStatus.mouseDelta.first * moveFactor;
				pitchDebug -= delta * moveStatus.mouseDelta.second * moveFactor;
				if (pitchDebug > 179.9) pitchDebug = 179.9; if (pitchDebug < -179.9) pitchDebug = -179.9;
				if (yawDebug > 89.9) yawDebug = 89.9; if (yawDebug < -89.9) yawDebug = -89.9;
				float dirX = cos(radians(yawDebug)) * cos(radians(pitchDebug));
				float dirY = sin(radians(pitchDebug));
				float dirZ = sin(radians(yawDebug)) * cos(radians(pitchDebug));
				dirVec = float_3(dirX, dirY, dirZ);
				moveStatus.mouseDelta.first = 0;
				moveStatus.mouseDelta.second = 0;
			}
			else{
				if (vulkanSystem.movementMode == MOVE_USER) {
					float moveFactor = 10;
					yaw += delta * moveStatus.mouseDelta.first * moveFactor;
					pitch -= delta * moveStatus.mouseDelta.second * moveFactor;
					if (pitch > 89.9) pitch = 89.9; if (pitch < -89.9) pitch = -89.9;
					if (yaw > 89.9) yaw = 89.9; if (yaw < -89.9) yaw = -89.9;
				}
				float dirX = cos(radians(yaw)) * cos(radians(pitch));
				float dirY = sin(radians(pitch));
				float dirZ = sin(radians(yaw)) * cos(radians(pitch));
				dirVec = float_3(dirX, dirY, dirZ);
				moveStatus.mouseDelta.first = 0;
				moveStatus.mouseDelta.second = 0;
			}
			if (vulkanSystem.movementMode == MOVE_DEBUG) {
				vulkanSystem.debugDirVec = dirVec;
				vulkanSystem.debugMoveVec = moveVec;
			}
			else {
				vulkanSystem.moveVec = moveVec;
				vulkanSystem.dirVec = dirVec;
			}
		}
		else {
			events.handleEventsQueue(delta);
		}
		//Update frame

		std::chrono::high_resolution_clock::time_point start =
			std::chrono::high_resolution_clock::now();
		vulkanSystem.drawFrame();
		if(animate) vulkanSystem.runDrivers(1 / 60.f, graph, true);
		std::chrono::high_resolution_clock::time_point end =
			std::chrono::high_resolution_clock::now();
		mscount += std::chrono::duration_cast<std::chrono::milliseconds>(
			end - start).count();
		framecount++;
		if (verbose && framecount == 1000) {
			std::cout << "MEASURE frametime (avg of 1000 frames): " << (float)
				mscount / 1000.f << "ms" << std::endl;
			mscount = 0;
			framecount = 0;
		}
	}

	vulkanSystem.idle();
}

int Mode::modeMain() {
	Texture defaultCube;
	defaultCube.doFree = false;
	defaultCube.realY = 6;
	defaultCube.y = 1;
	defaultCube.x = 1;
	defaultCube.type = defaultCube.TYPE_CUBE;
	defaultCube.format = defaultCube.FORM_RGBE;
	defaultCube.mipLevels = 1;
	unsigned char char255 = unsigned char(255);
	const unsigned char char255Arr[] = {
	char255 ,char255 ,char255 ,char255 ,
	 char255 ,char255 ,char255 ,char255 ,
	 char255 ,char255 ,char255 ,char255 ,
	 char255 ,char255 ,char255 ,char255 ,
	 char255 ,char255 ,char255 ,char255 ,
	 char255 ,char255 ,char255 ,char255 };
	defaultCube.data = char255Arr;
	Texture defaultShadow;
	defaultShadow.doFree = false;
	defaultShadow.realY = 1;
	defaultShadow.y = 1;
	defaultShadow.x = 1;
	defaultShadow.type = defaultShadow.TYPE_2D;
	defaultShadow.FORM_LIN;
	defaultShadow.mipLevels = 1;
	const unsigned char shadowArr[] = { char255,char255,char255,char255 };
	defaultShadow.data = shadowArr;

	//Parse and initialize user requested .s72 scene graph file
	Parser parser;
	SceneGraph graph = parser.parseJson(sceneName, verbose);

	graph.useInstancing = useInstancing;
	Texture lut = Texture::parseTexture("Textures/LUT.png", false);
	vulkanSystem.LUT = lut;

	DrawList drawList = graph.navigateSceneGraph(verbose, poolSize);
	if (drawList.cubeMaps.size() == 0) {
		drawList.cubeMaps.push_back(defaultCube);
	}
	if (drawList.textureMaps.size() == 0) {
		drawList.textureMaps.push_back(defaultShadow);
	}
	
	//Initialize Vulkan System
	vulkanSystem.shaderDir = shaderDir;
	vulkanSystem.useInstancing = drawList.instancedTransformPools.size() > 0 && useInstancing;
	vulkanSystem.mainWindow = mainWindow;
	vulkanSystem.deviceName = deviceName;
	vulkanSystem.useCulling = culling;
	vulkanSystem.poolSize = poolSize;
	vulkanSystem.platform = platform;
	vulkanSystem.defaultShadowTex = defaultShadow;

	std::chrono::high_resolution_clock::time_point initFirst = std::chrono::high_resolution_clock::now();
	vulkanSystem.initVulkan(drawList, cameraName);
	std::chrono::high_resolution_clock::time_point initLast = std::chrono::high_resolution_clock::now();
	if (verbose) std::cout << "MEASURE init vulkan: " << (float)
		std::chrono::duration_cast<std::chrono::milliseconds>(
			initLast - initFirst).count() << "ms" << std::endl;
	lastFrame = std::chrono::high_resolution_clock::now();
	movementMode = MOVE_USER;
	vulkanSystem.movementMode = movementMode;
	vulkanSystem.renderToWindow = true;

	//Handle headless mode
	if (!vulkanSystem.renderToWindow) {
		events = parser.parseEvents(eventName);
		events.vulkanSystemP = &vulkanSystem;
	}
	
	//Begin running main loop
	mainLoop(&graph);

	//Clean up vulkan
	//vulkanSystem.cleanup();
	return 0;
}
