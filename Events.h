#pragma once
#include <vector>
#include <string>
#include <iostream>
#include "VulkanSystem.h"

//Class to load and handle a file of headless events
enum EventsType {
	EV_PLAY,
	EV_AVAILABLE,
	EV_MARK,
	EV_SAVE, // IMPLEMENT LAST!
	EV_NONE
};

//Info related to animation control specifically
struct PlaybackInfo {
	float time;
	float rate;
};

//Event data structure (nearly a union but had to use a struct due to typing isssues)
struct Event
{
	int time = 0;
	EventsType type = EV_NONE ;
	PlaybackInfo playbackInfo;
	std::string saveName; //filename.ppm
	std::string markDescription;
	bool completed = false;
	Event(const Event& e) {
		time = e.time;
		type = e.type;
		time = e.time;
	};
	Event() {};
};

//Class to handle events
class HeadlessEvents {
public:
	HeadlessEvents() {};
	HeadlessEvents(VulkanSystem* vulkanSystem) {
		vulkanSystemP = vulkanSystem;
	};
	std::vector<Event> eventsQueue;
	//Given a frame time, when the next event is reached, handle it accordingly
	void handleEventsQueue(float delta) {
		if (vulkanSystemP == nullptr) {
			throw std::runtime_error("ERROR: Vulkan System pointer must be set before handling an events queue.");
		}
		int msDelta = delta * 1000;
		if (!eventsQueue[currentEvent].completed) return;
		if (currentEvent == eventsQueue.size() - 1 || eventsQueue[currentEvent + 1].time > msDelta) return;
		currentEvent++;
		switch (eventsQueue[currentEvent].type)
		{
		case EV_AVAILABLE:
			vulkanSystemP->headlessGuard = false;
			eventsQueue[currentEvent].completed = true;
			break;
		case EV_MARK:
			std::cout << eventsQueue[currentEvent].markDescription << std::endl;
			eventsQueue[currentEvent].completed = true;
			break;
		case EV_PLAY:
			vulkanSystemP->playbackSpeed = eventsQueue[currentEvent].playbackInfo.rate;
			vulkanSystemP->setDriverRuntime(eventsQueue[currentEvent].playbackInfo.time);
			eventsQueue[currentEvent].completed = true;
			break;
		case EV_SAVE:
			eventsQueue[currentEvent].completed = true;
			std::cout << "Saving to image not yet supported in Events." << std::endl;
			break;
		default:
			eventsQueue[currentEvent].completed = true;
			break;
		};
	};
	int currentEvent = 0;
	//Gives event handler direct access to vulkan to handle events and
	//in turn get info back from vulkan
	VulkanSystem* vulkanSystemP;
};