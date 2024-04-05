#include "VulkanSystem.h"
#include "vulkan/vulkan.h"
#include "FileHelp.h"
#include <iostream>
#include <optional>
#include "platform.h"
#ifdef PLATFORM_WIN
#include <Windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan_win32.h"
#endif // PLATFORM_WIN
#ifdef PLATFORM_LIN
#include "vulkan/vulkan_xcb.h"
#define VK_USE_PLATFORM_XCB_KHR
#endif //PLATFORM_LIN
#include <set>
#include <algorithm>
#include "MathHelpers.h"
#include <chrono>
#include "SceneGraph.h"
#include "shaderc/shaderc.hpp"
#include <thread>
#include "stb_image.h"
//https://vulkan-tutorial.com




static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void VulkanSystem::initVulkan(DrawList drawList, std::string cameraName) {
	//Vertex shader
	vertices = drawList.vertexPool;
	verticesInst = drawList.instancedVertexPool;
	indexPoolsStore = drawList.indexPools;
	indexInstPools = drawList.instancedIndexPools;
	transformPools = drawList.transformPools;
	transformInstPoolsStore = drawList.instancedTransformPools;
	transformInstIndexPools = drawList.instancedTransformIndexPools;
	
	//Materials
	transformNormalPools = drawList.normalTransformPools;
	transformNormalInstPoolsStore = drawList.instancedNormalTransformPools;
	transformEnvironmentPools = drawList.environmentTransformPools;
	transformEnvironmentInstPoolsStore = drawList.instancedEnvironmentTransformPools;
	materialPools = drawList.materialPools;
	instancedMaterials = drawList.instancedMaterials;
	rawTextures = drawList.textureMaps;
	rawCubes = drawList.cubeMaps;
	
	//Lights
	rawEnvironment = drawList.environmentMap;
	lightPool = drawList.lights;
	worldTolightPool = drawList.worldToLights;
	worldTolightPerspPool = drawList.worldToLightsPersp;

	//Animate and bounding box
	drawPools = drawList.drawPools;
	boundingSpheresInst = drawList.instancedBoundingSpheres;
	nodeDrivers = drawList.nodeDrivers;
	cameraDrivers = drawList.cameraDrivers;

	//Cameras
	cameras = drawList.cameras;

	int findCamera = 0;
	for (; findCamera < cameras.size(); findCamera++) {
		if (cameras[findCamera].name.compare(cameraName) == 0) break;
	}
	if (findCamera < cameras.size()) {
		currentCamera = findCamera;
	}

	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createAttachments();
	createImageViews();
	createRenderPasses();
	createDescriptorSetLayout();
	createGraphicsPipelines();
	createDepthResources();
	createFramebuffers();
	createCommands();
	createVertexBuffer();
	createTextureImages();
	createIndexBuffers();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();

	if (renderToWindow) {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, imageAvailableSemaphores.data() + i) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, renderFinishedSemaphores.data() + i) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, inFlightFences.data() + i) != VK_SUCCESS) {
				throw std::runtime_error("ERROR: Unable to create a semaphore or fence in VulkanSystem.");
			}
		}
	}

}

void VulkanSystem::listPhysicalDevices() {
	createInstance(false);
	std::cout << "Vulkan found the following supported physical devices: " << std::endl;

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		throw std::runtime_error("ERROR: No GPUs found with Vulkan support in VulkanSystem.");
	}
	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());
	for (const VkPhysicalDevice device : physicalDevices) {
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		std::cout << deviceProperties.deviceName << std::endl;
	}
}

bool updateChannelStep(Driver* driver, int ind, SceneGraph* sceneGraphP) {
	if (driver->lastIndex == ind) return false;
	float_3 translate = float_3(0,0,0);
	quaternion<float> rotate = quaternion<float>();
	float_3 scale = float_3(1, 1, 1);
	driver->lastIndex = ind;
	switch (driver->channel)
	{
	case CH_TRANSLATE:
		translate = float_3(
			driver->values[ind * 3],
			driver->values[ind * 3 + 1],
			driver->values[ind * 3 + 2]
		);
		sceneGraphP->graphNodes[driver->id].translate = translate;
		break;
	case CH_ROTATE:
		rotate.setAngle(driver->values[ind * 4 + 3]);
		rotate.setAxis(float_3(
			driver->values[ind * 4],
			driver->values[ind * 4 + 1],
			driver->values[ind * 4 + 2]
		));
		sceneGraphP->graphNodes[driver->id].rotation = rotate;
		break;
	default: //SCALE
		scale = float_3(
			driver->values[ind * 3],
			driver->values[ind * 3 + 1],
			driver->values[ind * 3 + 2]
		);
		sceneGraphP->graphNodes[driver->id].scale = scale;
		break;
	}
	return true;
}

bool updateChannelLinear(Driver* driver, int ind, SceneGraph* sceneGraphP) {
	int nextInd = ind + 1;
	if (nextInd >= driver->times.size()) nextInd = 0;
	float timeInd = driver->times[ind];
	float timeNext = driver->times[nextInd];
	float t = (driver->currentRuntime - timeInd) / (timeNext - timeInd);
	quaternion<float> rotateInd; quaternion<float> rotateNext;
	float_3 translateInd; float_3 translateNext;
	float_3 scaleInd; float_3 scaleNext;
	switch (driver->channel)
	{
	case CH_TRANSLATE:
		translateInd = float_3(
			driver->values[ind * 3],
			driver->values[ind * 3 + 1],
			driver->values[ind * 3 + 2]
		);
		translateNext = float_3(
			driver->values[nextInd * 3],
			driver->values[nextInd * 3 + 1],
			driver->values[nextInd * 3 + 2]
		);
		sceneGraphP->graphNodes[driver->id].translate = translateInd * (1 - t) + translateNext * t;
		break;
	case CH_ROTATE:
		rotateInd = quaternion<float>();
		rotateInd.setAngle(driver->values[ind * 4 + 3]);
		rotateInd.setAxis(float_3(
			driver->values[ind * 4],
			driver->values[ind * 4 + 1],
			driver->values[ind * 4 + 2]
		));
		rotateInd = rotateInd * (1 - t);
		rotateNext = quaternion<float>();
		rotateNext.setAngle(driver->values[nextInd * 4 + 3]);
		rotateNext.setAxis(float_3(
			driver->values[nextInd * 4],
			driver->values[nextInd * 4 + 1],
			driver->values[nextInd * 4 + 2]
		) );
		rotateNext = rotateNext * t;
		//This rotation math is correct, so, its a paramater problem
		sceneGraphP->graphNodes[driver->id].rotation = rotateInd + rotateNext;
		break;
	default: //SCALE
		scaleInd = float_3(
			driver->values[ind * 3],
			driver->values[ind * 3 + 1],
			driver->values[ind * 3 + 2]
		);
		scaleNext = float_3(
			driver->values[nextInd * 3],
			driver->values[nextInd * 3 + 1],
			driver->values[nextInd * 3 + 2]
		);
		sceneGraphP->graphNodes[driver->id].scale = scaleInd * (1 - t) + scaleNext * t;
		break;
	}
	return true;
}

bool updateChannelSlerp(Driver* driver, int ind, SceneGraph* sceneGraphP) {
	int nextInd = ind + 1;
	if (nextInd >= driver->times.size()) nextInd = 0;
	float timeInd = driver->times[ind];
	float timeNext = driver->times[nextInd];
	float t = (driver->currentRuntime - timeInd) / (timeNext - timeInd);
	float omega = 0; float constInd = 0; float constNext = 0; float dotAngle = 0;
	quaternion<float> rotateInd = quaternion<float>();
	quaternion<float> rotateNext = quaternion<float>();
	switch (driver->channel)
	{
	case CH_TRANSLATE:
		return updateChannelLinear(driver, ind, sceneGraphP);
		break;
	case CH_ROTATE:
		rotateInd = quaternion<float>();
		rotateInd.setAngle(driver->values[ind * 4 + 3] * (1 - t));
		rotateInd.setAxis(float_3(
			driver->values[ind * 4],
			driver->values[ind * 4 + 1],
			driver->values[ind * 4 + 2]
		) * (1 - t));
		rotateNext = quaternion<float>();
		rotateNext.setAngle(driver->values[nextInd * 4 + 3] * t);
		rotateNext.setAxis(float_3(
			driver->values[nextInd * 4],
			driver->values[nextInd * 4 + 1],
			driver->values[nextInd * 4 + 2]
		) * t);
		//https://en.wikipedia.org/wiki/Slerp
		dotAngle = rotateInd.normalize().dot(rotateNext.normalize());
		omega = acos(dotAngle);
		//Recalculate vectors without pre-interpolating
		rotateInd.setAngle(driver->values[ind * 4 + 3]);
		rotateInd.setAxis(float_3(
			driver->values[ind * 4],
			driver->values[ind * 4 + 1],
			driver->values[ind * 4 + 2]
		));
		rotateNext = quaternion<float>();
		rotateNext.setAngle(driver->values[nextInd * 4 + 3]);
		rotateNext.setAxis(float_3(
			driver->values[nextInd * 4],
			driver->values[nextInd * 4 + 1],
			driver->values[nextInd * 4 + 2]
		));
		constInd = sin((1 - t) * omega) / (sin(omega));
		constNext = sin(t * omega) / sin(omega);
		sceneGraphP->graphNodes[driver->id].rotation = (rotateInd*constInd) + (rotateNext*constNext);
		break;
	default: //SCALE
		return updateChannelLinear(driver, ind, sceneGraphP);
		break;
	}
	return true;
}

bool updateTransform(Driver* driver, float frameTime, SceneGraph* sceneGraphP, bool loop = false) {
	float lastTime = driver->currentRuntime;
	if(loop || driver->currentRuntime <= driver->times[driver->times.size() -1])driver->currentRuntime += frameTime;
	if (loop && driver->currentRuntime > driver->times[driver->times.size() - 1]) {
		driver->currentRuntime -= driver->times[driver->times.size() - 1];
	}
	if(driver->currentRuntime < driver->times[0]) 
		driver->currentRuntime += driver->times[driver->times.size() - 1];
	float thisTime = driver->currentRuntime;

	int ind = 0;
	for (; ind < driver->times.size(); ind++) {
		if (ind == driver->times.size() - 1 ||
			driver->times[ind + 1] >= thisTime) break;
	}
	switch (driver->interpolation)
	{
	case LINEAR:
		return updateChannelLinear(driver, ind, sceneGraphP);
		break;
	case STEP:
		return updateChannelStep(driver, ind, sceneGraphP);
		break; 
	default://SLERP
		return updateChannelSlerp(driver, ind, sceneGraphP);
		break;
	}
}

void VulkanSystem::runDrivers(float frameTime, SceneGraph* sceneGraphP, bool loop) {
	frameTime *= playbackSpeed; //1 when not in headless mode
	frameTime *= (playingAnimation ? (forwardAnimation ? 1 : -1) : 0);
	bool renavigate = false;
	for (size_t ind = 0; ind < nodeDrivers.size(); ind++) {
		Driver* driver = nodeDrivers.data() + ind;
		renavigate = updateTransform(driver, frameTime, sceneGraphP, loop) || renavigate;
	}
	for (size_t ind = 0; ind < cameraDrivers.size(); ind++) {
		Driver* driver = cameraDrivers.data() + ind;
		renavigate = renavigate || updateTransform(driver, frameTime, sceneGraphP, loop) || renavigate;
	}
	if (renavigate) {
		DrawList drawList = sceneGraphP->navigateSceneGraph(false, poolSize);
		transformPools = drawList.transformPools;
		transformInstPoolsStore = drawList.instancedTransformPools;
		transformNormalPools = drawList.normalTransformPools;
		transformNormalInstPoolsStore = drawList.instancedNormalTransformPools;
		transformEnvironmentPools = drawList.environmentTransformPools;
		transformEnvironmentInstPoolsStore = drawList.instancedEnvironmentTransformPools;
		cameras = drawList.cameras;
	}
}


void VulkanSystem::setDriverRuntime(float time) {
	for (size_t ind = 0; ind < cameraDrivers.size(); ind++) {
		cameraDrivers[ind].currentRuntime = time;
	}
	for (size_t ind = 0; ind < nodeDrivers.size(); ind++) {
		nodeDrivers[ind].currentRuntime = time;
	}
}

void VulkanSystem::cleanup() {
#ifndef NDEBUG
	std::cout << "Cleaning up Vulkan Mode." << std::endl;
#endif // DEBUG

	cleanupSwapChain();
	for (VkImageView texImageView : textureImageViews) {
		vkDestroyImageView(device, texImageView, nullptr);
	}
	for (VkImage texImage : textureImages) {
		vkDestroyImage(device, texImage, nullptr);
	}
	for (VkDeviceMemory texMemory : textureImageMemorys) {
		vkFreeMemory(device, texMemory, nullptr);
	}
	for (VkImageView texImageView : cubeImageViews) {
		vkDestroyImageView(device, texImageView, nullptr);
	}
	for (VkImage texImage : cubeImages) {
		vkDestroyImage(device, texImage, nullptr);
	}
	for (VkDeviceMemory texMemory : cubeImageMemorys) {
		vkFreeMemory(device, texMemory, nullptr);
	}
	if (rawEnvironment.has_value()) {
		vkDestroyImageView(device, environmentImageView, nullptr);
		vkDestroyImage(device, environmentImage, nullptr);
		vkFreeMemory(device, environmentImageMemory, nullptr);
	}
	vkDestroyImageView(device, LUTImageView, nullptr);
	vkDestroyImage(device, LUTImage, nullptr);
	vkFreeMemory(device, LUTImageMemory, nullptr);
	vkDestroyImageView(device, defaultShadowImageView, nullptr);
	vkDestroyImage(device, defaultShadowImage, nullptr);
	vkFreeMemory(device, defaultShadowImageMemory, nullptr);
	for (int pool = 0; pool < transformPools.size(); pool++) {
		for (int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
			vkDestroyBuffer(device, uniformBuffersTransformsPools[pool][frame], nullptr);
			vkFreeMemory(device, uniformBuffersMemoryTransformsPools[pool][frame], nullptr);
			vkDestroyBuffer(device, uniformBuffersEnvironmentTransformsPools[pool][frame], nullptr);
			vkFreeMemory(device, uniformBuffersMemoryEnvironmentTransformsPools[pool][frame], nullptr);
			vkDestroyBuffer(device, uniformBuffersNormalTransformsPools[pool][frame], nullptr);
			vkFreeMemory(device, uniformBuffersMemoryNormalTransformsPools[pool][frame], nullptr);
			vkDestroyBuffer(device, uniformBuffersMaterialsPools[pool][frame], nullptr);
			vkFreeMemory(device, uniformBuffersMemoryMaterialsPools[pool][frame], nullptr);
			vkDestroyBuffer(device, uniformBuffersCamerasPools[pool][frame], nullptr);
			vkFreeMemory(device, uniformBuffersMemoryCamerasPools[pool][frame], nullptr);
			vkDestroyBuffer(device, uniformBuffersLightsPools[pool][frame], nullptr);
			vkFreeMemory(device, uniformBuffersMemoryLightsPools[pool][frame], nullptr);
			vkDestroyBuffer(device, uniformBuffersLightTransformsPools[pool][frame], nullptr);
			vkFreeMemory(device, uniformBuffersMemoryLightTransformsPools[pool][frame], nullptr);
		}
	}
	vkDestroyDescriptorPool(device, descriptorPoolHDR, nullptr);
	for (int i = 0; i < lightPool.size(); i++) {
		vkDestroyDescriptorPool(device, descriptorPoolShadows[i], nullptr);
	}
	for (int i = 0; i < lightPool.size() + 2; i++) {
		vkDestroyDescriptorSetLayout(device, descriptorSetLayouts[i], nullptr);
	}
	for (int pool = 0; pool < indexBufferMemorys.size(); pool++) {
		vkDestroyBuffer(device, indexBuffers[pool], nullptr);
		vkFreeMemory(device, indexBufferMemorys[pool], nullptr);
	}
	for (int pool = 0; pool < indexInstBufferMemorys.size(); pool++) {
		vkDestroyBuffer(device, indexInstBuffers[pool], nullptr);
		vkFreeMemory(device, indexInstBufferMemorys[pool], nullptr);
	}
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
	vkDestroyBuffer(device, vertexInstBuffer, nullptr);
	vkFreeMemory(device, vertexInstBufferMemory, nullptr);
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipeline(device, graphicsInstPipeline, nullptr);
	for (int i = 0; i < lightPool.size(); i++) {
		vkDestroyPipelineLayout(device, pipelineLayoutShadows[i], nullptr);
	}
	vkDestroyPipelineLayout(device, pipelineLayoutHDR, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayoutFinal, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);
	for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}
	vkDestroyCommandPool(device, commandPool, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);

	if (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}
	//vkDestroyInstance(instance, nullptr); //Instance detroying crashes program
	//At the moment Id prefer to continue owning the application than
	//Handle a tiny amount of memory in the sole case of a closed window.
}


void VulkanSystem::createInstance(bool verbose) {
	if (enableValidationLayers && !checkValidationSupport()) {
		throw std::runtime_error("ERROR: One of the requested validation layers is not available in VulkanSystem.");
	}
	//Application info
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Back End";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Vulkan Back End";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	//Create info
	VkInstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &appInfo;

	//Extensions needed to support windows system
#ifdef PLATFORM_WIN


	std::array<const char*, 5> requiredExtensionArray = std::array<const char*, 5>();
	requiredExtensionArray[0] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
	requiredExtensionArray[1] = VK_KHR_SURFACE_EXTENSION_NAME;
	requiredExtensionArray[2] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	requiredExtensionArray[3] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
	requiredExtensionArray[4] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
	instanceCreateInfo.enabledExtensionCount = 5;
	instanceCreateInfo.ppEnabledExtensionNames = requiredExtensionArray.data();
#endif // PLATFORM_WIN

#ifdef PLATFORM_LIN

#endif // PLATFORM_LIN


	instanceCreateInfo.enabledLayerCount = 0;
	if (enableValidationLayers) {
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}

	if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("ERROR: Failed to create Vulkan instance in VulkanSystem.");
	}



	//Query supported extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
	if (verbose) {
		std::cout << "Available extensions:\n";
		for (const VkExtensionProperties& extension : availableExtensions) {
			std::cout << '\t' << extension.extensionName << std::endl;
		}
	}
}

void VulkanSystem::setupDebugMessenger() {
    if (!enableValidationLayers) return;
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("ERROR: Unable to set up debug messenger in VulkanSystem.");
	}
}

void VulkanSystem::createSurface() {
#ifdef PLATFORM_WIN


	VkWin32SurfaceCreateInfoKHR surfaceCreateInfoWin{};
	surfaceCreateInfoWin.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfoWin.hwnd = mainWindow->getHwnd();
	surfaceCreateInfoWin.hinstance = GetModuleHandle(nullptr);
	if (vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfoWin, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("ERROR: Failed to create surface for main window in VulkanSystem.");
	}
#endif // PLATFORM_WIN
#ifdef PLATFORM_LIN


	VkXcbSurfaceCreateInfoKHR surfaceCreateInfoLin{};
	surfaceCreateInfoLin.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfoLin.connection = mainWindow->getConnection();
	surfaceCreateInfoLin.window = mainWindow->getWindow();
	vkCreateXcbSurfaceKHR(instance, &surfaceCreateInfoLin, nullptr, &surface);
#endif // PLATFORM_LIN
}


bool VulkanSystem::checkValidationSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;
		for (const VkLayerProperties& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
			}
		}

		if (!layerFound) return false;
	}
	return true;
}


QueueFamilyIndices VulkanSystem::findQueueFamilies(VkPhysicalDevice device) {
	QueueFamilyIndices indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int currentIndex = 0;
	for (const VkQueueFamilyProperties& queueFamily : queueFamilies) {
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, currentIndex, surface, &presentSupport);
		if (presentSupport) {
			indices.presentFamily = currentIndex;
		}
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = currentIndex;
		}
		if (indices.isComplete()) break;
		currentIndex++;
	}
	return indices;
}

bool VulkanSystem::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
		nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
		availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (const VkExtensionProperties& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}
	return requiredExtensions.empty();
}


SwapChainSupportDetails VulkanSystem::querySwapChainSupport(VkPhysicalDevice device) {
	SwapChainSupportDetails queriedDetails;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
		&queriedDetails.capabilities);
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	if (formatCount != 0) {
		queriedDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, queriedDetails.formats.data());
	}
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		queriedDetails.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, queriedDetails.presentModes.data());
	}
	return queriedDetails;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {

		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentmodes) {
	for (const VkPresentModeKHR presentMode : availablePresentmodes) {
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return presentMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, std::pair<int, int> resolution) {
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	}
	else {
		VkExtent2D actualExtent = {
			static_cast<uint32_t>(resolution.first),
			static_cast<uint32_t>(resolution.second)
		};
		actualExtent.width = std::clamp(actualExtent.width,
			capabilities.minImageExtent.width,
			capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height,
			capabilities.minImageExtent.height,
			capabilities.maxImageExtent.height);
		return actualExtent;
	}
}

int VulkanSystem::isDeviceSuitable(VkPhysicalDevice device) {
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	if (!deviceFeatures.samplerAnisotropy) return -1;

	QueueFamilyIndices indices = findQueueFamilies(device);
	if (!indices.isComplete() || !CheckDeviceExtensionSupport(device)) {
		return -1;
	}
	bool swapChainSupported = false;
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
	if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) {
		std::cout << "ERROR: Failure in finding supported swapChain attributes in VulkanSystem." << std::endl;
		return -1;
	}
	int score = 0;
	//Maximally prefer a discrete GPU
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 100000;
	}

	//Prefer larger texter resolution and framebuffer size
	score += deviceProperties.limits.maxImageDimension2D;
	score += deviceProperties.limits.maxFramebufferHeight;
	score += deviceProperties.limits.maxFramebufferWidth;

	return score;
}


void VulkanSystem::pickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		throw std::runtime_error("ERROR: No GPUs found with Vulkan support in VulkanSystem.");
	}
	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());
	bool deviceFound = false;
	for (const VkPhysicalDevice device : physicalDevices) {
		int currentScore = isDeviceSuitable(device);
		if (currentScore > 0) {

			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);
			if (std::string(deviceProperties.deviceName).compare(deviceName) == 0) {
				deviceFound = true;
				physicalDevice = device;
			}
		}
	}
	if (!deviceFound) {
		throw std::runtime_error("ERROR: Unable to find user-requested device. Use the argument --list-physical-devices to see the names of all available devices.");
	}
}

void VulkanSystem::createLogicalDevice() {
	familyIndices = findQueueFamilies(physicalDevice);
	std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { familyIndices.graphicsFamily.value(), familyIndices.presentFamily.value() };
	float queuePriority = 1.0;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		deviceQueueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount =
		static_cast<uint32_t>(deviceQueueCreateInfos.size());
	createInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
	physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;
	createInfo.pEnabledFeatures = &physicalDeviceFeatures;
	createInfo.enabledExtensionCount =
		static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createInfo.enabledLayerCount = 0;
	if (enableValidationLayers) {
		createInfo.enabledLayerCount =
			static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device)
		!= VK_SUCCESS) {
		throw std::runtime_error("ERROR: Failure when trying to create logical device in VulkanSystem.");
	}
	vkGetDeviceQueue(device, familyIndices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, familyIndices.presentFamily.value(), 0, &presentQueue);
}

void VulkanSystem::createSwapChain() {
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkExtent2D extent;
	extent.width = mainWindow->resolution.first;
	extent.height = mainWindow->resolution.second;
	if (renderToWindow){
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		extent = chooseSwapExtent(swapChainSupport.capabilities, mainWindow->resolution);
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}


		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint32_t queueFamiliyIndices[] = { familyIndices.graphicsFamily.value(), familyIndices.presentFamily.value() };
		if (familyIndices.graphicsFamily != familyIndices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamiliyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("ERROR: Failed to create swap chain in VulkanSystem.");
		}

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
	}
	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

VkImageView VulkanSystem::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageViewType viewType, int levels) {
	VkImageViewCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = viewType;
	createInfo.format = format;
	VkComponentSwizzle swizzle = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.r = swizzle;
	createInfo.components.g = swizzle;
	createInfo.components.b = swizzle;
	createInfo.components.a = swizzle;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = levels;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(device, &createInfo, nullptr,&imageView)
		!= VK_SUCCESS) {
		throw std::runtime_error(
			std::string("ERROR: Failed to create an image view in VulkanSystem."));
	}
	return imageView;
}

//https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceMemoryProperties.html
int32_t VulkanSystem::getMemoryType(VkImage image) {
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(device, image, &memoryRequirements);
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t ind = 0; ind < memoryProperties.memoryTypeCount; ind++) {
		const uint32_t typeBits = 1 << ind;
		const bool isRequiredMemoryType = memoryRequirements.memoryTypeBits
			& typeBits;
		if (isRequiredMemoryType) return static_cast<uint32_t>(ind);
	}
	throw std::runtime_error("ERROR: Unable to find suitable memory type in VulkanSystem.");
}

void VulkanSystem::createAttachments() {
	VkExtent2D extent;
	extent.width = mainWindow->resolution.first;
	extent.height = mainWindow->resolution.second;
	attachmentImages.resize(swapChainImages.size());
	shadowImages.resize(lightPool.size());
	attachmentMemorys.resize(swapChainImages.size());
	shadowMemorys.resize(lightPool.size());

	for (int i = 0; i < lightPool.size(); i++) {
		shadowImages[i].resize(swapChainImages.size());
		shadowMemorys[i].resize(swapChainImages.size());
	}
	for (size_t image = 0; image < swapChainImages.size(); image++) {
		createImage(extent.width, extent.height, VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
			| VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, 0,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, attachmentImages[image],
			attachmentMemorys[image]);
		for (int i = 0; i < lightPool.size(); i++) {
			uint32_t shadowRes = lightPool[i].shadowRes;
			if (shadowRes == 0) shadowRes = 1;
			createImage(shadowRes, shadowRes, VK_FORMAT_B8G8R8A8_UNORM,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shadowImages[i][image],
				shadowMemorys[i][image]);
		}
	}
}


void VulkanSystem::createImageViews() {
	swapChainImageViews.resize(swapChainImages.size());
	for (size_t imageIndex = 0; imageIndex < swapChainImages.size();
		imageIndex++) {
		swapChainImageViews[imageIndex] = createImageView(
			swapChainImages[imageIndex], swapChainImageFormat);
	}
	attachmentImageViews.resize(attachmentImages.size());
	for (size_t imageIndex = 0; imageIndex < attachmentImages.size();
		imageIndex++) {
		attachmentImageViews[imageIndex] = createImageView(
			attachmentImages[imageIndex], VK_FORMAT_R32G32B32A32_SFLOAT);
	}
	shadowImageViews.resize(shadowImages.size());
	shadowSamplers.resize(shadowImages.size());
	for (int i = 0; i < shadowImages.size(); i++) {
		shadowImageViews[i].resize(shadowImages[i].size());
		shadowSamplers[i].resize(shadowImages[i].size());
	}
	for (size_t light = 0; light < shadowImages.size();
		light++) {
		//HERE
		for (int imageIndex = 0; imageIndex < shadowImages[light].size(); imageIndex++) {
			shadowImageViews[light][imageIndex] = createImageView(
				shadowImages[light][imageIndex], VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
			

			VkPhysicalDeviceProperties properties{};
			vkGetPhysicalDeviceProperties(physicalDevice, &properties);
			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy; //TODO: make this variable
			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.mipLodBias = 0.f;
			samplerInfo.minLod = 0;
			samplerInfo.maxLod = 0;

			if (vkCreateSampler(device, &samplerInfo, nullptr, &shadowSamplers[light][imageIndex]) != VK_SUCCESS) {
				throw std::runtime_error("ERROR: Unable to create a sampler in VulkanSystem.");
			}
		}
	}
}

void VulkanSystem::createRenderPasses() {
	shadowPasses.resize(lightPool.size());
	for (int i = 0; i < lightPool.size(); i++) {
		VkAttachmentDescription shadowAttachment = {};
		VkAttachmentReference shadowAttachmentRef = {};
		VkSubpassDescription shadowSubpass = {};


		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = VK_FORMAT_D16_UNORM;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		shadowAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
		shadowAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		shadowAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		shadowAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		shadowAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		shadowAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		shadowAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		shadowAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		shadowAttachmentRef.attachment = 0;
		shadowAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		shadowSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		shadowSubpass.colorAttachmentCount = 1;
		shadowSubpass.pColorAttachments = &shadowAttachmentRef;
		shadowSubpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkAttachmentDescription attachments[]{ shadowAttachment,depthAttachment };

		VkSubpassDependency shadowDependency;
		shadowDependency = {};
		shadowDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		shadowDependency.dstSubpass = 0;
		shadowDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		shadowDependency.srcAccessMask = 0;
		shadowDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		shadowDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 2;
		renderPassInfo.pAttachments = attachments;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &shadowSubpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &shadowDependency;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &shadowPasses[i]) != VK_SUCCESS) {
			throw std::runtime_error("ERROR: Was unable to create render pass in VulkanSystem.");
		}
	}

	std::vector<VkSubpassDescription> finalSubpasses = std::vector<VkSubpassDescription>(2);
	for (int i = 0; i < 2; i++) finalSubpasses[i] = {};

	VkAttachmentDescription colorAttachmentHDR{};
	colorAttachmentHDR.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	colorAttachmentHDR.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentHDR.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachmentHDR.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentHDR.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentHDR.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentHDR.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentHDR.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference colorAttachmentRefHDR{};
	colorAttachmentRefHDR.attachment = 2;
	colorAttachmentRefHDR.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachmentFinal{};
	colorAttachmentFinal.format = swapChainImageFormat;
	colorAttachmentFinal.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentFinal.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachmentFinal.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentFinal.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentFinal.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentFinal.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentFinal.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRefFinal{};
	colorAttachmentRefFinal.attachment = 0;
	colorAttachmentRefFinal.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = VK_FORMAT_D32_SFLOAT;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	
	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	finalSubpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	finalSubpasses[0].colorAttachmentCount = 1;
	finalSubpasses[0].pColorAttachments = &colorAttachmentRefHDR;
	finalSubpasses[0].pDepthStencilAttachment = &depthAttachmentRef;
	finalSubpasses[0].inputAttachmentCount = 0;
	finalSubpasses[0].pInputAttachments = nullptr;

	//https://www.saschawillems.de/blog/2018/07/19/vulkan-input-attachments-and-sub-passes/
	VkAttachmentReference inputRef;
	inputRef.attachment = 2;
	inputRef.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	
	finalSubpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	finalSubpasses[1].colorAttachmentCount = 1;
	finalSubpasses[1].pColorAttachments = &colorAttachmentRefFinal;
	finalSubpasses[1].inputAttachmentCount = 1;
	finalSubpasses[1].pInputAttachments = &inputRef;

	//Dependency for subpass before render pass
	std::vector<VkSubpassDependency> dependencies = std::vector<VkSubpassDependency>(2);

	//Stencil buffer pass to render pass
	dependencies[0] = {};
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	//Render pass to present pass
	dependencies[1] = {};
	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = 1;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	std::vector<VkAttachmentDescription> attachments;
	attachments.push_back(colorAttachmentFinal);
	attachments.push_back(depthAttachment);
	attachments.push_back(colorAttachmentHDR);
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = attachments.size();
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = finalSubpasses.size();
	renderPassInfo.pSubpasses = finalSubpasses.data();
	renderPassInfo.dependencyCount = dependencies.size();
	renderPassInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("ERROR: Was unable to create render pass in VulkanSystem.");
	}
}

void VulkanSystem::createDescriptorSetLayout() {

	descriptorSetLayouts.resize(2 + lightPool.size());

	for (int i = 0; i < lightPool.size(); i++) {
		VkDescriptorSetLayoutBinding meshBinding{};
		meshBinding.binding = 0;
		meshBinding.descriptorCount = 1;
		meshBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		meshBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfoShadow{};
		layoutInfoShadow.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfoShadow.bindingCount = 1;
		layoutInfoShadow.pBindings = &meshBinding;
		if (vkCreateDescriptorSetLayout(
			device, &layoutInfoShadow, nullptr, &descriptorSetLayouts[i]) != VK_SUCCESS) {
			throw std::runtime_error("ERROR: Failed to create a descriptor set layout in Vulkan System.");
		}
	}


	
	VkDescriptorSetLayoutBinding transformBinding{};
	transformBinding.binding = 0;
	transformBinding.descriptorCount = 1;
	transformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	transformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding cameraBinding{};
	cameraBinding.binding = 1;
	cameraBinding.descriptorCount = 1;
	cameraBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	cameraBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding materialBinding{};
	materialBinding.binding = 2;
	materialBinding.descriptorCount = 1;
	materialBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	materialBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding textureBinding{};
	textureBinding.binding = 3;
	textureBinding.descriptorCount = rawTextures.size(); 
	textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureBinding.pImmutableSamplers = nullptr;
	textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding cubeBinding{};
	cubeBinding.binding = 4;
	cubeBinding.descriptorCount = rawCubes.size();
	cubeBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	cubeBinding.pImmutableSamplers = nullptr;
	cubeBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding LUTBinding{};
	LUTBinding.binding = 5;
	LUTBinding.descriptorCount = 1;
	LUTBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	LUTBinding.pImmutableSamplers = nullptr;
	LUTBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;



	VkDescriptorSetLayoutBinding lightTransformBinding{};
	lightTransformBinding.binding = 6;
	lightTransformBinding.descriptorCount = 1;
	lightTransformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	lightTransformBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding lightBinding{};
	lightBinding.binding = 7;
	lightBinding.descriptorCount = 1;
	lightBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	lightBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;



	VkDescriptorSetLayoutBinding shadowMapBinding{};
	shadowMapBinding.binding = 8;
	shadowMapBinding.descriptorCount = lightPool.size();
	shadowMapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	shadowMapBinding.pImmutableSamplers = nullptr;
	shadowMapBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


	VkDescriptorSetLayoutBinding lightPerspectiveBinding{};
	lightPerspectiveBinding.binding = 9;
	lightPerspectiveBinding.descriptorCount = 1;
	lightPerspectiveBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	lightPerspectiveBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding environmentBinding{};
	environmentBinding.binding = 10;
	environmentBinding.descriptorCount = 1;
	environmentBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	environmentBinding.pImmutableSamplers = nullptr;
	environmentBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding normTransformBinding{};
	normTransformBinding.binding = 11;
	normTransformBinding.descriptorCount = 1;
	normTransformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	normTransformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding envTransformBinding{};
	envTransformBinding.binding = 12;
	envTransformBinding.descriptorCount = 1;
	envTransformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	envTransformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;


	VkDescriptorSetLayoutBinding bindings[] = { 
		transformBinding, cameraBinding, materialBinding,textureBinding, 
		cubeBinding, LUTBinding, lightTransformBinding, lightBinding, shadowMapBinding, lightPerspectiveBinding,
		environmentBinding, normTransformBinding, envTransformBinding};

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = rawEnvironment.has_value() ? 13 : 10;
	layoutInfo.pBindings = bindings;
	if (vkCreateDescriptorSetLayout(
		device, &layoutInfo, nullptr, &descriptorSetLayouts[lightPool.size()]) != VK_SUCCESS) {
		throw std::runtime_error("ERROR: Failed to create a descriptor set layout in Vulkan System.");
	}

	VkDescriptorSetLayoutBinding hdrBinding{};
	hdrBinding.binding = 0;
	hdrBinding.descriptorCount = 1;
	hdrBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	hdrBinding.pImmutableSamplers = nullptr;
	hdrBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfoFinal{};
	layoutInfoFinal.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfoFinal.bindingCount = 1;
	layoutInfoFinal.pBindings = &hdrBinding;
	if (vkCreateDescriptorSetLayout(
		device, &layoutInfoFinal, nullptr, &descriptorSetLayouts[lightPool.size() + 1]) != VK_SUCCESS) {
		throw std::runtime_error("ERROR: Failed to create a descriptor set layout in Vulkan System.");
	}


}

void VulkanSystem::createGraphicsPipeline(std::string vertShader, 
	std::string fragShader, VkPipeline& pipeline, VkPipelineLayout& layout, 
	int subpass, VkRenderPass inRenderPass) {
	std::vector<char> vertexShaderRawData = readFile((shaderDir + vertShader).c_str());
	std::vector<char> fragmentShaderRawData = readFile((shaderDir + fragShader).c_str());

	VkShaderModule vertexShaderModule = createShaderModule(vertexShaderRawData);
	VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderRawData);

	VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
	vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderStageInfo.module = vertexShaderModule;
	vertexShaderStageInfo.pName = "main";
	VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
	fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderStageInfo.module = fragmentShaderModule;
	fragmentShaderStageInfo.pName = "main";
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageInfo, fragmentShaderStageInfo };

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 6>
		attributeDescriptions = Vertex::getAttributeDescriptions();
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();


	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = false;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //Anything else requires GPU feature to be enabled
	rasterizer.lineWidth = 1.0f; //NEEDS wideLines to be enabled if > 1.0
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = false; //Add to depth value, not using

	//Currently not multisampling, will enable later
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = true;
	depthStencil.depthWriteEnable = true;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;


	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.layout = layout;
	pipelineInfo.renderPass = inRenderPass;
	pipelineInfo.subpass = subpass;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("ERROR: Unable to create graphics pipeline in VulkanSystem.");
	}

	vkDestroyShaderModule(device, vertexShaderModule, nullptr);
	vkDestroyShaderModule(device, fragmentShaderModule, nullptr);

}

void VulkanSystem::createGraphicsPipelines() {
	size_t subpassCount = 2 + lightPool.size();
	pipelineLayoutShadows.resize(lightPool.size());
	graphicsPipelineShadows.resize(lightPool.size());
	graphicsInstPipelineShadows.resize(lightPool.size());

	for (int i = 0; i < lightPool.size(); i++) {

		VkPushConstantRange lightTransformConstant;
		lightTransformConstant.offset = 0;
		lightTransformConstant.size = sizeof(mat44<float>);
		lightTransformConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		VkPipelineLayoutCreateInfo pipelineLayoutInfoShadow{};
		pipelineLayoutInfoShadow.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfoShadow.setLayoutCount = 1;
		pipelineLayoutInfoShadow.pSetLayouts = &descriptorSetLayouts[i];
		pipelineLayoutInfoShadow.pushConstantRangeCount = 1;
		pipelineLayoutInfoShadow.pPushConstantRanges = &lightTransformConstant;
		if (vkCreatePipelineLayout(device, &pipelineLayoutInfoShadow, nullptr, &pipelineLayoutShadows[i]) != VK_SUCCESS) {
			throw std::runtime_error("ERROR: Unable to create pipeline layout in VulkanSystems.");
		}
		createGraphicsPipeline("/vertShadow.spv", "/fragShadow.spv", graphicsPipelineShadows[i], pipelineLayoutShadows[i], 0, shadowPasses[i]);

		createGraphicsPipeline("/vertShadowInst.spv", "/fragShadow.spv", graphicsInstPipelineShadows[i], pipelineLayoutShadows[i],0, shadowPasses[i]);

	}

	VkPushConstantRange numLightsConstant;
	numLightsConstant.offset = 0;
	numLightsConstant.size = sizeof(int) + sizeof(float)*3 + sizeof(float);
	numLightsConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


	VkPipelineLayoutCreateInfo pipelineLayoutInfoHDR{};
	pipelineLayoutInfoHDR.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfoHDR.setLayoutCount = 1;
	pipelineLayoutInfoHDR.pSetLayouts = &descriptorSetLayouts[lightPool.size()];
	pipelineLayoutInfoHDR.pushConstantRangeCount = 1;
	pipelineLayoutInfoHDR.pPushConstantRanges = &numLightsConstant;

	VkPipelineLayoutCreateInfo pipelineLayoutInfoFinal{};
	pipelineLayoutInfoFinal.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfoFinal.setLayoutCount = 1;
	pipelineLayoutInfoFinal.pSetLayouts = &descriptorSetLayouts[lightPool.size() + 1];
	pipelineLayoutInfoFinal.pushConstantRangeCount = 0;
	pipelineLayoutInfoFinal.pPushConstantRanges = nullptr;


	if (vkCreatePipelineLayout(device, &pipelineLayoutInfoHDR, nullptr, &pipelineLayoutHDR) != VK_SUCCESS) {
		throw std::runtime_error("ERROR: Unable to create pipeline layout in VulkanSystems.");
	}

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfoFinal, nullptr, &pipelineLayoutFinal) != VK_SUCCESS) {
		throw std::runtime_error("ERROR: Unable to create pipeline layout in VulkanSystems.");
	}

	if (rawEnvironment.has_value()) {
		createGraphicsPipeline("/vertEnv.spv", "/fragEnv.spv", graphicsPipeline, pipelineLayoutHDR, subpassCount-2,renderPass);

		createGraphicsPipeline("/vertInstEnv.spv", "/fragEnv.spv", graphicsInstPipeline, pipelineLayoutHDR, 0, renderPass);
	}
	else {
		createGraphicsPipeline("/vert.spv", "/frag.spv", graphicsPipeline, pipelineLayoutHDR, 0, renderPass);
		createGraphicsPipeline("/vertInst.spv", "/frag.spv", graphicsInstPipeline, pipelineLayoutHDR, 0, renderPass);
	}
	createGraphicsPipeline("/vertQuad.spv", "/fragFinal.spv", graphicsPipelineFinal, pipelineLayoutFinal, 1, renderPass);
}

VkShaderModule VulkanSystem::createShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("ERROR: Failed to create a shader module in VulkanSystem.");
	}
	return shaderModule;
}

void VulkanSystem::createFramebuffers() {
	swapChainFramebuffers.resize(swapChainImageViews.size());
	shadowFramebuffers.resize(lightPool.size());
	for (int i = 0; i < lightPool.size(); i++) {
		shadowFramebuffers[i].resize(swapChainImageViews.size());
	}
	for (size_t image = 0; image < swapChainImageViews.size(); image++) {
		for (int i = 0; i < lightPool.size(); i++) {

			size_t shadowRes = lightPool[i].shadowRes;
			if (shadowRes == 0) shadowRes = 1;
			VkImageView attachments[] = { shadowImageViews[i][image],shadowDepthImageViews[i]};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = shadowPasses[i];
			framebufferInfo.attachmentCount = 2;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = shadowRes;
			framebufferInfo.height = shadowRes;
			framebufferInfo.layers = 1;
			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
				&(shadowFramebuffers[i][image])) != VK_SUCCESS) {
				throw std::runtime_error("ERROR: Unable to create a framebuffer in VulkanSystem.");
			}
		}


		std::vector<VkImageView> attachments;
		attachments.push_back(swapChainImageViews[image]);
		attachments.push_back(depthImageView);
		attachments.push_back(attachmentImageViews[image]);

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = attachments.size();
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;
		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, 
			&(swapChainFramebuffers[image])) != VK_SUCCESS) {
			throw std::runtime_error("ERROR: Unable to create a framebuffer in VulkanSystem.");
		}
	}
}

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, 
	VkPhysicalDevice physicalDevice) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	for (uint32_t index = 0; index < memProperties.memoryTypeCount; index++) {
		if ((typeFilter & (1 << index)) && (memProperties.memoryTypes[index].propertyFlags & properties) == properties) {
			return index;
		}
	}
	throw std::runtime_error("ERROR: Unable to find a suitable memory type in VulkanSystem.");
}


void VulkanSystem::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
	VkMemoryPropertyFlags properties, VkBuffer& buffer, 
	VkDeviceMemory& bufferMemory, bool realloc) {
	if (size == 0) return;
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (realloc)
	{
		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("ERROR: Creating a buffer in VulkanSystem.");
		}
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
		properties, physicalDevice);
	if (realloc) {
		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("ERROR: Unable to allocate a buffer memory in VulkanSystem.");
		}
		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}
}

VkCommandBuffer VulkanSystem::beginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VulkanSystem::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void VulkanSystem::copyBuffer(VkBuffer source, VkBuffer dest,
	VkDeviceSize size) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, source, dest, 1, &copyRegion);
	endSingleTimeCommands(commandBuffer);
}

void VulkanSystem::createVertexBuffer(bool realloc) {
	useVertexBuffer = false;
	int stagingBits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	int vertexUsageBits = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	int vertexPropertyBits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	if (vertices.size() != 0) {
		useVertexBuffer = true;

		//Create temp staging buffer
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
		VkBuffer stagingBuffer{};
		VkDeviceMemory stagingBufferMemory{};
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBits,
			stagingBuffer, stagingBufferMemory, true);

		//Move vertex data to GPU
		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		//Create proper vertex buffer
		createBuffer(bufferSize, vertexUsageBits, vertexPropertyBits,
			vertexBuffer, vertexBufferMemory, realloc);
		copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	if (useInstancing) {
		//Create temp staging buffer
		VkDeviceSize bufferInstSize = sizeof(verticesInst[0]) * verticesInst.size();
		VkBuffer stagingInstBuffer{};
		VkDeviceMemory stagingInstBufferMemory{};

		createBuffer(bufferInstSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBits,
			stagingInstBuffer, stagingInstBufferMemory, true);

		//Move vertex data to GPU
		void* data;
		vkMapMemory(device, stagingInstBufferMemory, 0, bufferInstSize, 0, &data);
		memcpy(data, verticesInst.data(), (size_t)bufferInstSize);
		vkUnmapMemory(device, stagingInstBufferMemory);

		//Create proper vertex buffer
		createBuffer(bufferInstSize, vertexUsageBits, vertexPropertyBits,
			vertexInstBuffer, vertexInstBufferMemory, realloc);
		copyBuffer(stagingInstBuffer, vertexInstBuffer, bufferInstSize);
		vkDestroyBuffer(device, stagingInstBuffer, nullptr);
		vkFreeMemory(device, stagingInstBufferMemory, nullptr);
	}
}

//Pre-calculate info needed for all points
struct frustumInfo {
	float_3 topNormal;
	float_3 bottomNormal;
	float_3 nearNormal;
	float_3 farNormal;
	float_3 leftNormal;
	float_3 rightNormal;
	float_3 topOrigin;
	float_3 bottomOrigin;
	float_3 nearOrigin;
	float_3 farOrigin;
	float_3 leftOrigin;
	float_3 rightOrigin;
	float nearBottom; 
	float nearTop; 
	float nearLeft;
	float nearRight;
	float farTop; 
	float farLeft;
	float farRight;
	float farBottom; 
	float farZ; 
	float nearZ; 
};

frustumInfo findFrustumInfo(DrawCamera camera) {
	frustumInfo info;
	//Constraints
	info.farZ = camera.perspectiveInfo.farP;
	info.nearZ = camera.perspectiveInfo.nearP;
	info.nearTop = tan(camera.perspectiveInfo.vfov / 2.0) * info.nearZ;
	info.farTop = tan(camera.perspectiveInfo.vfov / 2.0) * info.farZ;
	info.nearBottom = -tan(camera.perspectiveInfo.vfov / 2.0) * info.nearZ;
	info.farBottom = -tan(camera.perspectiveInfo.vfov / 2.0) * info.farZ;
	info.nearRight = camera.perspectiveInfo.aspect * info.nearTop;
	info.farRight = camera.perspectiveInfo.aspect * info.farTop;
	info.nearLeft = camera.perspectiveInfo.aspect * info.nearBottom;
	info.farLeft = camera.perspectiveInfo.aspect * info.farBottom;
	//Normals
	info.nearNormal = float_3(0, 0, 1);
	info.farNormal = float_3(0, 0, -1);
	info.rightNormal = (
		float_3(info.farRight, info.farTop, info.farZ) -
		float_3(info.nearRight, info.nearTop, info.nearZ)).cross(
			float_3(0, -1, 0)
		).normalize();
	info.leftNormal = (
		float_3(info.farLeft, info.farTop, info.farZ) -
		float_3(info.nearLeft, info.nearTop, info.nearZ)).cross(
			float_3(0, 1, 0)
		).normalize();
	info.topNormal = (
		float_3(info.farRight, info.farTop, info.farZ) -
		float_3(info.nearRight, info.nearTop, info.nearZ)).cross(
			float_3(-1, 0, 0)
		).normalize();
	info.bottomNormal = (
		float_3(info.farRight, info.farBottom, info.farZ) -
		float_3(info.nearRight, info.nearBottom, info.nearZ)).cross(
			float_3(1, 0, 0)
		).normalize();
	//Origins
	info.nearOrigin = float_3(0, 0, info.nearZ);
	info.farOrigin = float_3(0, 0, info.farZ);
	float midZ = info.nearZ + (info.farZ - info.nearZ) / 2.0;
	info.topOrigin = float_3(0, info.nearTop + 0.5 * (info.farTop - info.nearTop), midZ);
	info.bottomOrigin = float_3(0, info.nearBottom + 0.5 * (info.farBottom - info.nearBottom), midZ);
	info.leftOrigin = float_3(info.nearLeft + 0.5 * (info.farLeft - info.nearLeft), 0, midZ);
	info.rightOrigin = float_3(info.nearRight + 0.5 * (info.farRight - info.nearRight), 0, midZ);
	return info;
}

bool sphereInFrustum(std::pair<float_3, float> boundingSphere, frustumInfo info, mat44<float> toCameraSpace, mat44<float> toWorldSpace) {
	//rotate point into camera space
	float_3 rotatedPoint = toCameraSpace * toWorldSpace * boundingSphere.first;
	//Nan check (case when camera is unintialized)
	if (rotatedPoint.x != rotatedPoint.x || rotatedPoint.y != rotatedPoint.y || rotatedPoint.z != rotatedPoint.z) return false;
	rotatedPoint.z *= -1;

	float_3 fromOrigin;
	float originDist;
	//Project the point on to each plane
	//Near
	fromOrigin = rotatedPoint - info.nearOrigin;
	originDist = info.nearNormal.dot(fromOrigin);
	float_3 nearPoint = rotatedPoint - info.nearNormal * originDist;
	//bool belowNearBound = 

	//Far
	fromOrigin = rotatedPoint - info.farOrigin;
	originDist = info.farNormal.dot(fromOrigin);
	float_3 farPoint = rotatedPoint - info.farNormal * originDist;

	//Right
	fromOrigin = rotatedPoint - info.rightOrigin;
	originDist = info.rightNormal.dot(fromOrigin);
	float_3 rightPoint = rotatedPoint - info.rightNormal * originDist;

	//Left
	fromOrigin = rotatedPoint - info.leftOrigin;
	originDist = info.leftNormal.dot(fromOrigin);
	float_3 leftPoint = rotatedPoint - info.leftNormal * originDist;

	//Top
	fromOrigin = rotatedPoint - info.topOrigin;
	originDist = info.topNormal.dot(fromOrigin);
	float_3 topPoint = rotatedPoint - info.topNormal * originDist;

	//Bottom
	fromOrigin = rotatedPoint - info.bottomOrigin;
	originDist = info.bottomNormal.dot(fromOrigin);
	float_3 bottomPoint = rotatedPoint - info.bottomNormal * originDist;
	 

	//Re-project point into frustum constraints
	
	//Near
	if (nearPoint.x < info.nearLeft) nearPoint.x = info.nearLeft;
	if (nearPoint.x > info.nearRight) nearPoint.x = info.nearRight;
	if (nearPoint.y < info.nearBottom) nearPoint.y = info.nearBottom;
	if (nearPoint.y > info.nearTop) nearPoint.y = info.nearTop;

	//Far
	if (farPoint.x < info.farLeft) farPoint.x = info.farLeft;
	if (farPoint.x > info.farRight) farPoint.x = info.farRight;
	if (farPoint.y < info.farBottom) farPoint.y = info.farBottom;
	if (farPoint.y > info.farTop) farPoint.y = info.farTop;

	//Right
	if (rightPoint.z > info.farZ) {
		rightPoint.z = info.farZ;
	}
	else if (rightPoint.z < info.nearZ) {
		rightPoint.z = info.nearZ;
	}
	float zPercentage = (rightPoint.z - info.nearZ) / (info.farZ - info.nearZ);
	float rightTop = info.nearTop + (info.farTop - info.nearTop) * zPercentage;
	float rightBottom = info.nearBottom + (info.farBottom - info.nearBottom) * zPercentage;
	if (rightPoint.y > rightTop) {
		rightPoint.y = rightTop;
	}
	else if (rightPoint.y < rightBottom) {
		rightPoint.y = rightBottom;
	}
	fromOrigin = rightPoint - info.rightOrigin;
	originDist = info.rightNormal.dot(fromOrigin);
	rightPoint = rightPoint - info.rightNormal * originDist;

	//Left
	if (leftPoint.z > info.farZ) {
		leftPoint.z = info.farZ;
	}
	else if (leftPoint.z < info.nearZ) {
		leftPoint.z = info.nearZ;
	}
	zPercentage = (leftPoint.z - info.nearZ) / (info.farZ - info.nearZ);
	float leftTop = info.nearTop + (info.farTop - info.nearTop) * zPercentage;
	float leftBottom = info.nearBottom + (info.farBottom - info.nearBottom) * zPercentage;
	if (leftPoint.y > leftTop) {
		leftPoint.y = leftTop;
	}
	else if (leftPoint.y < leftBottom) {
		leftPoint.y = leftBottom;
	}
	fromOrigin = leftPoint - info.leftOrigin;
	originDist = info.leftNormal.dot(fromOrigin);
	leftPoint = leftPoint - info.leftNormal * originDist;

	//Top
	if (topPoint.z > info.farZ) {
		topPoint.z = info.farZ;
	}
	else if (topPoint.z < info.nearZ) {
		topPoint.z = info.nearZ;
	}
	zPercentage = (topPoint.z - info.nearZ) / (info.farZ - info.nearZ);
	float topLeft = info.nearLeft + (info.farLeft - info.nearLeft) * zPercentage;
	float topRight = info.nearRight + (info.farRight - info.nearRight) * zPercentage;
	if (topPoint.x > topRight) {
		topPoint.x = topRight;
	}
	else if (topPoint.x < topLeft) {
		topPoint.x = topLeft;
	}
	fromOrigin = topPoint - info.topOrigin;
	originDist = info.topNormal.dot(fromOrigin);
	topPoint = topPoint - info.topNormal * originDist;

	//Bottom
	if (bottomPoint.z > info.farZ) {
		bottomPoint.z = info.farZ;
	}
	else if (bottomPoint.z < info.nearZ) {
		bottomPoint.z = info.nearZ;
	}
	zPercentage = (bottomPoint.z - info.nearZ) / (info.farZ - info.nearZ);
	float bottomLeft = info.nearLeft + (info.farLeft - info.nearLeft) * zPercentage;
	float bottomRight = info.nearRight + (info.farRight - info.nearRight) * zPercentage;
	if (bottomPoint.x > bottomRight) {
		bottomPoint.x = bottomRight;
	}
	else if (bottomPoint.x < bottomLeft) {
		bottomPoint.x = bottomLeft;
	}
	fromOrigin = bottomPoint - info.bottomOrigin;
	originDist = info.bottomNormal.dot(fromOrigin);
	bottomPoint = bottomPoint - info.bottomNormal * originDist;

	//If ANY re-projected point is within radius, then there is an intersection
	if ((rotatedPoint - nearPoint).norm() <= boundingSphere.second) {
		return true;
	}
	if ((rotatedPoint - farPoint).norm() <= boundingSphere.second) {
		return true;
	}
	if ((rotatedPoint - topPoint).norm() <= boundingSphere.second) {
		return true;
	}
	if ((rotatedPoint - bottomPoint).norm() <= boundingSphere.second) {
		return true;
	}
	if ((rotatedPoint - leftPoint).norm() <= boundingSphere.second) {
		return true;
	}
	if ((rotatedPoint - rightPoint).norm() <= boundingSphere.second) {
		return true;
	}

	//Check if inside frustum
	if (
		rotatedPoint.x < rightPoint.x && rotatedPoint.x > leftPoint.x &&
		rotatedPoint.y < topPoint.y && rotatedPoint.y > bottomPoint.y &&
		rotatedPoint.z > info.nearZ && rotatedPoint.z < info.farZ
		) {
		return true;
	}

	return false;
}

mat44<float> VulkanSystem::getCameraSpace(DrawCamera camera, float_3 useMoveVec, float_3 useDirVec) {
	useDirVec = useDirVec.normalize()*-1;
	float_3 up = float_3(0, 0, 1);
	float_3 cameraRight = up.cross(useDirVec);
	float_3 cameraUp = useDirVec.cross(cameraRight);
	cameraRight = cameraRight.normalize(); cameraUp = cameraUp.normalize();
	vec4 transposed0 = float_4(cameraRight[0], cameraUp[0], useDirVec[0], 0).normalize();
	vec4 transposed1 = float_4(cameraRight[1], cameraUp[1], useDirVec[1], 0).normalize();
	vec4 transposed2 = float_4(cameraRight[2], cameraUp[2], useDirVec[2], 0).normalize();
	mat44 localRot = mat44<float>(transposed0, transposed1, transposed2, float_4(0, 0, 0, 1));
	float_3 moveLocal = float_3(useMoveVec.x, useMoveVec.y, useMoveVec.z);
	mat44 local = mat44<float>(localRot);
	local.data[3][0] = moveLocal.x;
	local.data[3][1] = moveLocal.y;
	local.data[3][2] = moveLocal.z;
	local = local * camera.transform;
	return local;
}

void VulkanSystem::cullInstances() {

	if (transformInstPools.size() < transformInstPoolsStore.size()) {
		transformInstPools = std::vector<std::vector<mat44<float>>>(transformInstPoolsStore.size());
		transformEnvironmentInstPools = std::vector<std::vector<mat44<float>>>(transformEnvironmentInstPoolsStore.size());
		transformNormalInstPools = std::vector<std::vector<mat44<float>>>(transformNormalInstPoolsStore.size());
	}
	DrawCamera camera = cameras[currentCamera];
	frustumInfo info = findFrustumInfo(camera);
	mat44<float> cameraSpace = getCameraSpace(camera, moveVec, dirVec);
	for (size_t pool = 0; pool < transformInstPools.size(); pool++) {
		transformInstPools[pool].clear();
		transformInstPools[pool].reserve(transformInstPoolsStore[pool].size());
		if (rawEnvironment.has_value()) {
			transformEnvironmentInstPools[pool].clear();
			transformEnvironmentInstPools[pool].reserve(transformInstPoolsStore[pool].size());
		}
		transformNormalInstPools[pool].clear();
		transformNormalInstPools[pool].reserve(transformInstPoolsStore[pool].size());
		for (size_t transform = 0; transform < transformInstPoolsStore[pool].size(); transform++) {
			if (!useCulling || sphereInFrustum(boundingSpheresInst[transformInstIndexPools[pool]], info, cameraSpace, transformInstPoolsStore[pool][transform])) {
				transformInstPools[pool].push_back(transformInstPoolsStore[pool][transform]);
				if (rawEnvironment.has_value()) {
					transformEnvironmentInstPools[pool].push_back(transformEnvironmentInstPoolsStore[pool][transform]);
				}
				transformNormalInstPools[pool].push_back(transformNormalInstPoolsStore[pool][transform]);
			}
		}
	}
}

void VulkanSystem::cullIndexPools() {
	if (indexPools.size() < indexPoolsStore.size()) {
		indexPools = std::vector<std::vector<uint32_t>>(indexPoolsStore.size());
		indexBuffersValid = std::vector<bool>(indexPoolsStore.size());
	}
	DrawCamera camera = cameras[currentCamera];
	frustumInfo info = findFrustumInfo(camera);
	mat44<float> cameraSpace = getCameraSpace(camera, moveVec, dirVec);
	for (size_t pool = 0; pool < drawPools.size(); pool++) {
		indexPools[pool].clear();
		indexPools[pool].reserve(indexPoolsStore[pool].size());
		for (size_t node = 0; node < drawPools[pool].size(); node++) {
			if (!useCulling || sphereInFrustum(drawPools[pool][node].boundingSphere, info, cameraSpace, transformPools[pool][node])) {
				int indexBegin = drawPools[pool][node].indexStart;
				int indexEnd = drawPools[pool][node].indexCount + indexBegin;
				indexPools[pool].insert(
					indexPools[pool].end(),
					indexPoolsStore[pool].begin() + indexBegin,
					indexPoolsStore[pool].begin() + indexEnd);
			}
		}
	}
}

void VulkanSystem::transitionImageLayout(VkImage image, VkFormat format,
	VkImageLayout oldLayout, VkImageLayout newLayout, int layers, int levels) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = levels;
	barrier.subresourceRange.layerCount = layers;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destStage;
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout
		== VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::runtime_error("ERROR: Invalid arguments passed into layout transition in VulkanSystem.");
	}

	
	vkCmdPipelineBarrier(commandBuffer, sourceStage, destStage, 0, 0, 
		nullptr, 0, nullptr, 1, &barrier);

	endSingleTimeCommands(commandBuffer);
}


void VulkanSystem::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, int level , int face) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferImageHeight = 0;
	region.bufferRowLength = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = level;
	region.imageSubresource.baseArrayLayer = face;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0,0,0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	endSingleTimeCommands(commandBuffer);
}

//https://vulkan-tutorial.com/Generating_Mipmaps
void VulkanSystem::generateMipmaps(VkImage image, int32_t x, int32_t y, uint32_t mipLevels, int face) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = face;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int mipX = x; int mipY = y;
	for (int mipLevel = 1; mipLevel < mipLevels; mipLevel++) {
		int lastMipLevel = mipLevel - 1;
		barrier.subresourceRange.baseMipLevel = lastMipLevel;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
			&barrier);

		//Blit mipmap
		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0,0,0 };
		blit.srcOffsets[1] = { mipX,mipY,1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = lastMipLevel;
		blit.srcSubresource.baseArrayLayer = face;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = {
			mipX > 1 ? mipX / 2 : 1,
			mipY > 1 ? mipY / 2 : 1,
			1
		};
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = mipLevel;
		blit.dstSubresource.baseArrayLayer = face;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);


		if (mipX > 1) mipX /= 2; if (mipY > 1) mipY /= 2;
	}
	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
		&barrier);

	endSingleTimeCommands(commandBuffer);
}


void VulkanSystem::createEnvironmentImage(Texture env, VkImage& image,
	VkDeviceMemory& memory, VkImageView& imageView, VkSampler& sampler) {

	VkDeviceSize imageSize = 4 * env.x * env.realY;
	VkDeviceSize layerSize = 4 * env.x * env.y;
	std::array<VkBuffer,6> stageBuffer;
	std::array<VkDeviceMemory,6> stageBufferMemory;
	for (int face = 0; face < 6; face++) {
		createBuffer(imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stageBuffer[face], stageBufferMemory[face], true);
		void* data;
		vkMapMemory(device, stageBufferMemory[face], 0, layerSize, 0, &data);
		memcpy(data, (void*)(&env.data[face*static_cast<size_t>(layerSize)]), static_cast<size_t>(layerSize));
		vkUnmapMemory(device, stageBufferMemory[face]);
	}
	if (env.doFree) {
		stbi_image_free((void*)env.data);
	}

	int usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	int flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	createImage(env.x, env.y, VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL, usage, flags,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory,6,env.mipLevels);
	transitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,6,env.mipLevels);
	for (int face = 0; face < 6; face++) {
		copyBufferToImage(stageBuffer[face], image, static_cast<uint32_t>(env.x), static_cast<uint32_t>(env.y), 0, face);
		generateMipmaps(image, env.x, env.y, env.mipLevels, face);
		vkDestroyBuffer(device, stageBuffer[face], nullptr);
		vkFreeMemory(device, stageBufferMemory[face], nullptr);
	}


	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.layerCount = 6;
	viewInfo.subresourceRange.levelCount = env.mipLevels;
	if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("ERROR: Unable to create a texture image view in VulkanSystem.");
	}
	

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy; //TODO: make this variable
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.f;
	samplerInfo.minLod = 0;
	samplerInfo.maxLod = env.mipLevels;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
		throw std::runtime_error("ERROR: Unable to create a sampler in VulkanSystem.");
	}
	int a = 0;
}

void VulkanSystem::createTextureImage(Texture tex, VkImage& image, 
	VkDeviceMemory& memory, VkImageView& imageView, VkSampler& sampler) {
	VkDeviceSize imageSize = 4 * tex.x * tex.realY;
	VkBuffer stageBuffer;
	VkDeviceMemory stageBufferMemory;
	createBuffer(imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stageBuffer, stageBufferMemory, true);
	void* data;
	vkMapMemory(device, stageBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, (void*)tex.data, static_cast<size_t>(imageSize));
	vkUnmapMemory(device, stageBufferMemory);
	if (tex.doFree) {
		stbi_image_free((void*)tex.data);
	}

	int usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	createImage(tex.x, tex.realY, VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,usage ,0, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory, 1,tex.mipLevels);
	transitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,tex.mipLevels);
	copyBufferToImage(stageBuffer, image, static_cast<uint32_t>(tex.x), static_cast<uint32_t>(tex.realY));
	generateMipmaps(image, tex.x, tex.realY, tex.mipLevels);

	vkDestroyBuffer(device, stageBuffer, nullptr);
	vkFreeMemory(device, stageBufferMemory, nullptr);

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.layerCount = 1;
	viewInfo.subresourceRange.levelCount = tex.mipLevels;
	if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("ERROR: Unable to create a texture image view in VulkanSystem.");
	}

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR; 
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy; //TODO: make this variable
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.f;
	samplerInfo.minLod = 0;
	samplerInfo.maxLod = static_cast<float>(tex.mipLevels);

	if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler)!=VK_SUCCESS) {
		throw std::runtime_error("ERROR: Unable to create a sampler in VulkanSystem.");
	}
}

void VulkanSystem::createTextureImages() {
	textureImages.resize(rawTextures.size());
	textureImageMemorys.resize(rawTextures.size());
	textureImageViews.resize(rawTextures.size());
	textureSamplers.resize(rawTextures.size());
	for (size_t texInd = 0; texInd < rawTextures.size(); texInd++) {
		Texture tex = rawTextures[texInd];
		createTextureImage(
			tex,
			textureImages[texInd],
			textureImageMemorys[texInd],
			textureImageViews[texInd],
			textureSamplers[texInd]
		);
	}
	createTextureImage(LUT, LUTImage, LUTImageMemory, LUTImageView, LUTSampler);
	createTextureImage(defaultShadowTex, defaultShadowImage, defaultShadowImageMemory, defaultShadowImageView, defaultShadowSampler);
	cubeImages.resize(rawCubes.size());
	cubeImageMemorys.resize(rawCubes.size());
	cubeImageViews.resize(rawCubes.size());
	cubeSamplers.resize(rawCubes.size());
	for (size_t cubeInd = 0; cubeInd < rawCubes.size(); cubeInd++) {
		Texture tex = rawCubes[cubeInd];
		createEnvironmentImage(
			tex,
			cubeImages[cubeInd],
			cubeImageMemorys[cubeInd],
			cubeImageViews[cubeInd],
			cubeSamplers[cubeInd]
		);
	}
	if (rawEnvironment.has_value()) {
		createEnvironmentImage(
			*rawEnvironment,
			environmentImage,
			environmentImageMemory,
			environmentImageView,
			environmentSampler
		);
	}
	int a = 0;

}


void VulkanSystem::createIndexBuffers(bool realloc, bool andFree) {
	if (andFree) {
		for (int pool = 0; pool < indexBufferMemorys.size(); pool++) {
			if (!indexBuffersValid[pool]) continue;
			vkDestroyBuffer(device, indexBuffers[pool], nullptr);
			vkFreeMemory(device, indexBufferMemorys[pool], nullptr);
		}
		for (int pool = 0; pool < indexInstBufferMemorys.size(); pool++) {
			vkDestroyBuffer(device, indexInstBuffers[pool], nullptr);
			vkFreeMemory(device, indexInstBufferMemorys[pool], nullptr);
		}
	}
	if (useVertexBuffer) {
		cullIndexPools();
		for (int pool = 0; pool < indexPools.size(); pool++) {
			if (indexPools[pool].size() == 0) {
				indexBuffersValid[pool] = false;
			}
			else {
				indexBuffersValid[pool] = true;
			}
		}

		indexBufferMemorys.resize(indexPools.size());
		indexBuffers.resize(indexPools.size());
		for (size_t pool = 0; pool < indexPools.size(); pool++) {
			if (!indexBuffersValid[pool]) continue;
			VkDeviceSize bufferSize = sizeof(indexPools[pool][0]) * indexPools[pool].size();
			if (bufferSize > 0) {

				//Create temp staging buffer
				VkBuffer stagingBuffer{};
				VkDeviceMemory stagingBufferMemory{};
				int stagingBits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
				createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBits,
					stagingBuffer, stagingBufferMemory, true);

				//Move index data to GPU
				void* data;
				vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
				memcpy(data, indexPools[pool].data(), (size_t)bufferSize);
				vkUnmapMemory(device, stagingBufferMemory);

				//Create proper index buffer
				int indexUsageBits = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
				int indexPropertyBits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
				createBuffer(bufferSize, indexUsageBits, indexPropertyBits,
					indexBuffers[pool], indexBufferMemorys[pool], realloc);
				copyBuffer(stagingBuffer, indexBuffers[pool], bufferSize);

				vkDestroyBuffer(device, stagingBuffer, nullptr);
				vkFreeMemory(device, stagingBufferMemory, nullptr);
			}
		}
	}
	if (useInstancing) {
		indexInstBufferMemorys.resize(indexInstPools.size());
		indexInstBuffers.resize(indexInstPools.size());
		for (size_t pool = 0; pool < indexInstPools.size(); pool++) {
			VkDeviceSize bufferSize = sizeof(indexInstPools[pool][0]) * indexInstPools[pool].size();
			if (bufferSize > 0) {

				//Create temp staging buffer
				VkBuffer stagingBuffer{};
				VkDeviceMemory stagingBufferMemory{};
				int stagingBits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
				createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBits,
					stagingBuffer, stagingBufferMemory, true);

				//Move index data to GPU
				void* data;
				vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
				memcpy(data, indexInstPools[pool].data(), (size_t)bufferSize);
				vkUnmapMemory(device, stagingBufferMemory);

				//Create proper index buffer
				int indexUsageBits = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
				int indexPropertyBits = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
				createBuffer(bufferSize, indexUsageBits, indexPropertyBits,
					indexInstBuffers[pool], indexInstBufferMemorys[pool], realloc);
				copyBuffer(stagingBuffer, indexInstBuffers[pool], bufferSize);

				vkDestroyBuffer(device, stagingBuffer, nullptr);
				vkFreeMemory(device, stagingBufferMemory, nullptr);
			}
		}
	}
}

void VulkanSystem::createUniformBuffers(bool realloc) {
	cullInstances();

	size_t transformsSize = useVertexBuffer ?
		transformPools.size() : 0;
	transformsSize = useInstancing ?
		transformsSize + transformInstPools.size() :
		transformsSize;
	uniformBuffersTransformsPools.resize(transformsSize);
	uniformBuffersMemoryTransformsPools.resize(transformsSize);
	uniformBuffersMappedTransformsPools.resize(transformsSize);
	uniformBuffersLightPerspectivePools.resize(transformsSize);
	uniformBuffersMemoryLightPerspectivePools.resize(transformsSize);
	uniformBuffersMappedLightPerspectivePools.resize(transformsSize);
	uniformBuffersModelsPools.resize(lightPool.size());
	uniformBuffersMemoryModelsPools.resize(lightPool.size());
	uniformBuffersMappedModelsPools.resize(lightPool.size());
	for (int i = 0; i < lightPool.size(); i++) {
		uniformBuffersModelsPools[i].resize(transformsSize);
		uniformBuffersMemoryModelsPools[i].resize(transformsSize);
		uniformBuffersMappedModelsPools[i].resize(transformsSize);
	}
	if (rawEnvironment.has_value()) {
		uniformBuffersEnvironmentTransformsPools.resize(transformsSize);
		uniformBuffersMemoryEnvironmentTransformsPools.resize(transformsSize);
		uniformBuffersMappedEnvironmentTransformsPools.resize(transformsSize);
		uniformBuffersNormalTransformsPools.resize(transformsSize);
		uniformBuffersMemoryNormalTransformsPools.resize(transformsSize);
		uniformBuffersMappedNormalTransformsPools.resize(transformsSize);
	}
	uniformBuffersCamerasPools.resize(transformsSize);
	uniformBuffersMemoryCamerasPools.resize(transformsSize);
	uniformBuffersMappedCamerasPools.resize(transformsSize);
	uniformBuffersLightsPools.resize(transformsSize);
	uniformBuffersMemoryLightsPools.resize(transformsSize);
	uniformBuffersMappedLightsPools.resize(transformsSize);
	uniformBuffersLightTransformsPools.resize(transformsSize);
	uniformBuffersMemoryLightTransformsPools.resize(transformsSize);
	uniformBuffersMappedLightTransformsPools.resize(transformsSize);

	uniformBuffersMaterialsPools.resize(transformsSize);
	uniformBuffersMemoryMaterialsPools.resize(transformsSize);
	uniformBuffersMappedMaterialsPools.resize(transformsSize);
	for (size_t pool = 0; pool < transformsSize; pool++) {
		uniformBuffersTransformsPools[pool].resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemoryTransformsPools[pool].resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMappedTransformsPools[pool].resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersLightPerspectivePools[pool].resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemoryLightPerspectivePools[pool].resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMappedLightPerspectivePools[pool].resize(MAX_FRAMES_IN_FLIGHT);

		for (int i = 0; i < lightPool.size(); i++) {
			uniformBuffersModelsPools[i][pool].resize(MAX_FRAMES_IN_FLIGHT);
			uniformBuffersMemoryModelsPools[i][pool].resize(MAX_FRAMES_IN_FLIGHT);
			uniformBuffersMappedModelsPools[i][pool].resize(MAX_FRAMES_IN_FLIGHT);
		}
		if (rawEnvironment.has_value()) {
			uniformBuffersEnvironmentTransformsPools[pool].resize(MAX_FRAMES_IN_FLIGHT);
			uniformBuffersMemoryEnvironmentTransformsPools[pool].resize(MAX_FRAMES_IN_FLIGHT);
			uniformBuffersMappedEnvironmentTransformsPools[pool].resize(MAX_FRAMES_IN_FLIGHT);
			uniformBuffersNormalTransformsPools[pool].resize(MAX_FRAMES_IN_FLIGHT);
			uniformBuffersMemoryNormalTransformsPools[pool].resize(MAX_FRAMES_IN_FLIGHT);
			uniformBuffersMappedNormalTransformsPools[pool].resize(MAX_FRAMES_IN_FLIGHT);
		}
		uniformBuffersCamerasPools[pool].resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemoryCamerasPools[pool].resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMappedCamerasPools[pool].resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersLightsPools[pool].resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemoryLightsPools[pool].resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMappedLightsPools[pool].resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersLightTransformsPools[pool].resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemoryLightTransformsPools[pool].resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMappedLightTransformsPools[pool].resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMaterialsPools[pool].resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemoryMaterialsPools[pool].resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMappedMaterialsPools[pool].resize(MAX_FRAMES_IN_FLIGHT);
	}

	int props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	for (int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
		size_t pool = 0;
		for (; pool < transformPools.size() && useVertexBuffer; pool++)
		{
			VkDeviceSize bufferSizeTransforms = sizeof(mat44<float>) * transformPools[pool].size();
			VkDeviceSize bufferSizeNormalTransforms = sizeof(mat44<float>) * transformPools[pool].size();
			VkDeviceSize bufferSizeEnvironmentTransforms;
			if (rawEnvironment.has_value()) bufferSizeEnvironmentTransforms= sizeof(mat44<float>) * transformEnvironmentPools[pool].size();
			VkDeviceSize bufferSizeCameras = sizeof(mat44<float>);
			VkDeviceSize bufferSizeLights = sizeof(Light) * lightPool.size();
			VkDeviceSize bufferSizeLightTransforms = sizeof(mat44<float>) * lightPool.size();
			VkDeviceSize bufferSizeMaterials = sizeof(DrawMaterial) * materialPools[pool].size();
			
			for (int i = 0; i < lightPool.size(); i++) {
				createBuffer(bufferSizeTransforms, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, props,
					uniformBuffersModelsPools[i][pool][frame], uniformBuffersMemoryModelsPools[i][pool][frame], realloc);
				vkMapMemory(device, uniformBuffersMemoryModelsPools[i][pool][frame], 0, bufferSizeTransforms, 0,
					uniformBuffersMappedModelsPools[i][pool].data() + frame);
			}
			createBuffer(bufferSizeTransforms, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, props,
				uniformBuffersTransformsPools[pool][frame], uniformBuffersMemoryTransformsPools[pool][frame], realloc);
			vkMapMemory(device, uniformBuffersMemoryTransformsPools[pool][frame], 0, bufferSizeTransforms, 0,
				uniformBuffersMappedTransformsPools[pool].data() + frame);
			createBuffer(bufferSizeLightTransforms, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, props,
				uniformBuffersLightPerspectivePools[pool][frame], uniformBuffersMemoryLightPerspectivePools[pool][frame], realloc);
			vkMapMemory(device, uniformBuffersMemoryLightPerspectivePools[pool][frame], 0, bufferSizeLightTransforms, 0,
				uniformBuffersMappedLightPerspectivePools[pool].data() + frame);
			if (rawEnvironment.has_value()) {
				createBuffer(bufferSizeNormalTransforms, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, props,
					uniformBuffersNormalTransformsPools[pool][frame], uniformBuffersMemoryNormalTransformsPools[pool][frame], realloc);
				vkMapMemory(device, uniformBuffersMemoryNormalTransformsPools[pool][frame], 0, bufferSizeNormalTransforms, 0,
					uniformBuffersMappedNormalTransformsPools[pool].data() + frame);
				createBuffer(bufferSizeEnvironmentTransforms, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, props,
					uniformBuffersEnvironmentTransformsPools[pool][frame], uniformBuffersMemoryEnvironmentTransformsPools[pool][frame], realloc);
				vkMapMemory(device, uniformBuffersMemoryEnvironmentTransformsPools[pool][frame], 0, bufferSizeEnvironmentTransforms, 0,
					uniformBuffersMappedEnvironmentTransformsPools[pool].data() + frame);
			}
			createBuffer(bufferSizeCameras, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, props,
				uniformBuffersCamerasPools[pool][frame], uniformBuffersMemoryCamerasPools[pool][frame], realloc);
			vkMapMemory(device, uniformBuffersMemoryCamerasPools[pool][frame], 0, bufferSizeCameras, 0,
				uniformBuffersMappedCamerasPools[pool].data() + frame);
			if (lightPool.size() > 0) {
				createBuffer(bufferSizeLights, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, props,
					uniformBuffersLightsPools[pool][frame], uniformBuffersMemoryLightsPools[pool][frame], realloc);
				vkMapMemory(device, uniformBuffersMemoryLightsPools[pool][frame], 0, bufferSizeLights, 0,
					uniformBuffersMappedLightsPools[pool].data() + frame);
				createBuffer(bufferSizeLightTransforms, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, props,
					uniformBuffersLightTransformsPools[pool][frame], uniformBuffersMemoryLightTransformsPools[pool][frame], realloc);
				vkMapMemory(device, uniformBuffersMemoryLightTransformsPools[pool][frame], 0, bufferSizeLightTransforms, 0,
					uniformBuffersMappedLightTransformsPools[pool].data() + frame);
			}
			createBuffer(bufferSizeMaterials, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, props,
				uniformBuffersMaterialsPools[pool][frame], uniformBuffersMemoryMaterialsPools[pool][frame], realloc);
			vkMapMemory(device, uniformBuffersMemoryMaterialsPools[pool][frame], 0, bufferSizeMaterials, 0,
				uniformBuffersMappedMaterialsPools[pool].data() + frame);
		}
		for (;pool < transformsSize && useInstancing; pool++)
		{
			//Unfortunately, the results of culling cant be used here, every possible transform needs to be accounted for in the buffer size
			//Even if they end up unused!
			VkDeviceSize bufferSizeTransforms = sizeof(mat44<float>) * transformInstPoolsStore[pool - transformPools.size()].size();
			VkDeviceSize bufferSizeNormalTransforms = sizeof(mat44<float>) * transformInstPoolsStore[pool - transformPools.size()].size();
			VkDeviceSize bufferSizeEnvironmentTransforms;
			if (rawEnvironment.has_value())bufferSizeEnvironmentTransforms = sizeof(mat44<float>) * transformEnvironmentInstPoolsStore[pool - transformPools.size()].size();
			VkDeviceSize bufferSizeCameras = sizeof(mat44<float>);
			VkDeviceSize bufferSizeLights = sizeof(Light) * lightPool.size();
			VkDeviceSize bufferSizeLightTransforms = sizeof(mat44<float>) * lightPool.size();
			VkDeviceSize bufferSizeMaterials = sizeof(DrawMaterial);
			createBuffer(bufferSizeTransforms, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, props,
				uniformBuffersTransformsPools[pool][frame], uniformBuffersMemoryTransformsPools[pool][frame], realloc);
			vkMapMemory(device, uniformBuffersMemoryTransformsPools[pool][frame], 0, bufferSizeTransforms, 0,
				uniformBuffersMappedTransformsPools[pool].data() + frame);
			createBuffer(bufferSizeLightTransforms, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, props,
				uniformBuffersLightPerspectivePools[pool][frame], uniformBuffersMemoryLightPerspectivePools[pool][frame], realloc);
			vkMapMemory(device, uniformBuffersMemoryLightPerspectivePools[pool][frame], 0, bufferSizeLightTransforms, 0,
				uniformBuffersMappedLightPerspectivePools[pool].data() + frame);
			for (int i = 0; i < lightPool.size(); i++) {
				createBuffer(bufferSizeTransforms, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, props,
					uniformBuffersModelsPools[i][pool][frame], uniformBuffersMemoryModelsPools[i][pool][frame], realloc);
				vkMapMemory(device, uniformBuffersMemoryModelsPools[i][pool][frame], 0, bufferSizeTransforms, 0,
					uniformBuffersMappedModelsPools[i][pool].data() + frame);
			}
			if (rawEnvironment.has_value()) {
				createBuffer(bufferSizeNormalTransforms, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, props,
					uniformBuffersNormalTransformsPools[pool][frame], uniformBuffersMemoryNormalTransformsPools[pool][frame], realloc);
				vkMapMemory(device, uniformBuffersMemoryNormalTransformsPools[pool][frame], 0, bufferSizeNormalTransforms, 0,
					uniformBuffersMappedNormalTransformsPools[pool].data() + frame);
				createBuffer(bufferSizeEnvironmentTransforms, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, props,
					uniformBuffersEnvironmentTransformsPools[pool][frame], uniformBuffersMemoryEnvironmentTransformsPools[pool][frame], realloc);
				vkMapMemory(device, uniformBuffersMemoryEnvironmentTransformsPools[pool][frame], 0, bufferSizeEnvironmentTransforms, 0,
					uniformBuffersMappedEnvironmentTransformsPools[pool].data() + frame);
			}
			createBuffer(bufferSizeCameras, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, props,
				uniformBuffersCamerasPools[pool][frame], uniformBuffersMemoryCamerasPools[pool][frame], realloc);
			vkMapMemory(device, uniformBuffersMemoryCamerasPools[pool][frame], 0, bufferSizeCameras, 0,
				uniformBuffersMappedCamerasPools[pool].data() + frame);
			

			if (lightPool.size() > 0) {
				createBuffer(bufferSizeLights, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, props,
					uniformBuffersLightsPools[pool][frame], uniformBuffersMemoryLightsPools[pool][frame], realloc);
				vkMapMemory(device, uniformBuffersMemoryLightsPools[pool][frame], 0, bufferSizeLights, 0,
					uniformBuffersMappedLightsPools[pool].data() + frame);
				createBuffer(bufferSizeLightTransforms, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, props,
					uniformBuffersLightTransformsPools[pool][frame], uniformBuffersMemoryLightTransformsPools[pool][frame], realloc);
				vkMapMemory(device, uniformBuffersMemoryLightTransformsPools[pool][frame], 0, bufferSizeLightTransforms, 0,
					uniformBuffersMappedLightTransformsPools[pool].data() + frame);
			}
			createBuffer(bufferSizeMaterials, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, props,
				uniformBuffersMaterialsPools[pool][frame], uniformBuffersMemoryMaterialsPools[pool][frame], realloc);
			vkMapMemory(device, uniformBuffersMemoryMaterialsPools[pool][frame], 0, bufferSizeMaterials, 0,
				uniformBuffersMappedMaterialsPools[pool].data() + frame);
		}
	}
}


void VulkanSystem::createDescriptorPool() {
	size_t transformsSize = useVertexBuffer ?
		transformPools.size() : 0;
	transformsSize = useInstancing ?
		transformsSize + transformInstPools.size() :
		transformsSize;

	descriptorPoolShadows.resize(lightPool.size());
	for (int i = 0; i < lightPool.size(); i++) {
		VkDescriptorPoolSize poolSizeShadow{};
		poolSizeShadow.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		poolSizeShadow.descriptorCount = MAX_FRAMES_IN_FLIGHT * (transformsSize);
		VkDescriptorPoolCreateInfo poolInfoShadow{};
		poolInfoShadow.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfoShadow.poolSizeCount = 1;
		poolInfoShadow.pPoolSizes = &poolSizeShadow;
		poolInfoShadow.maxSets = (transformsSize)*MAX_FRAMES_IN_FLIGHT;
		if (vkCreateDescriptorPool(device, &poolInfoShadow, nullptr, &descriptorPoolShadows[i])
			!= VK_SUCCESS) {
			throw std::runtime_error("ERROR: Unable to create a descriptor pool in Vulkan System.");
		}
	}

	std::array<VkDescriptorPoolSize,2> poolSizesHDR{};
	poolSizesHDR[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizesHDR[0].descriptorCount = rawEnvironment.has_value() ? 
		8 * (transformsSize)*MAX_FRAMES_IN_FLIGHT :
		6 * (transformsSize)*MAX_FRAMES_IN_FLIGHT;
	poolSizesHDR[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizesHDR[1].descriptorCount = rawEnvironment.has_value() ? 
		(rawTextures.size() + rawCubes.size() + 1 + lightPool.size()) * transformsSize*MAX_FRAMES_IN_FLIGHT : 
		(rawTextures.size() + rawCubes.size() + lightPool.size()) * transformsSize * MAX_FRAMES_IN_FLIGHT;
	VkDescriptorPoolCreateInfo poolInfoHDR{};
	poolInfoHDR.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfoHDR.poolSizeCount = poolSizesHDR.size();
	poolInfoHDR.pPoolSizes = poolSizesHDR.data();
	poolInfoHDR.maxSets = (transformsSize) * MAX_FRAMES_IN_FLIGHT;
	if (vkCreateDescriptorPool(device, &poolInfoHDR, nullptr, &descriptorPoolHDR)
		!= VK_SUCCESS) {
		throw std::runtime_error("ERROR: Unable to create a descriptor pool in Vulkan System.");
	}
	VkDescriptorPoolSize poolSizeFinal{};
	poolSizeFinal.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	poolSizeFinal.descriptorCount = MAX_FRAMES_IN_FLIGHT;
	VkDescriptorPoolCreateInfo poolInfoFinal{};
	poolInfoFinal.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfoFinal.poolSizeCount = 1;
	poolInfoFinal.pPoolSizes = &poolSizeFinal;
	poolInfoFinal.maxSets = MAX_FRAMES_IN_FLIGHT;
	if (vkCreateDescriptorPool(device, &poolInfoFinal, nullptr, &descriptorPoolFinal)
		!= VK_SUCCESS) {
		throw std::runtime_error("ERROR: Unable to create a descriptor pool in Vulkan System.");
	}
}

void VulkanSystem::createDescriptorSets() {

	size_t transformsSize = useVertexBuffer ?
		transformPools.size() : 0;
	transformsSize = useInstancing ?
		transformsSize + transformInstPools.size() :
		transformsSize;




	std::vector<VkDescriptorSetLayout> layoutsHDR(MAX_FRAMES_IN_FLIGHT *
		transformsSize, descriptorSetLayouts[lightPool.size()]);
	VkDescriptorSetAllocateInfo allocateInfoHDR{};
	allocateInfoHDR.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfoHDR.descriptorPool = descriptorPoolHDR;
	allocateInfoHDR.descriptorSetCount = MAX_FRAMES_IN_FLIGHT * (transformsSize);
	allocateInfoHDR.pSetLayouts = layoutsHDR.data();
	descriptorSetsHDR.resize(MAX_FRAMES_IN_FLIGHT * (transformsSize));
	if (vkAllocateDescriptorSets(device, &allocateInfoHDR, descriptorSetsHDR.data())
		!= VK_SUCCESS) {
		throw std::runtime_error("ERROR: Unable to create descriptor sets in Vulkan System. HDR.");
	}

	std::vector<VkDescriptorSetLayout> layoutsFinal(MAX_FRAMES_IN_FLIGHT, descriptorSetLayouts[lightPool.size() + 1]);
	VkDescriptorSetAllocateInfo allocateInfoFinal{};
	allocateInfoFinal.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfoFinal.descriptorPool = descriptorPoolFinal;
	allocateInfoFinal.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
	allocateInfoFinal.pSetLayouts = layoutsFinal.data();
	descriptorSetsFinal.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(device, &allocateInfoFinal, descriptorSetsFinal.data())
		!= VK_SUCCESS) {
		throw std::runtime_error("ERROR: Unable to create descriptor sets in Vulkan System. Final.");
	}

	descriptorSetsShadows.resize(lightPool.size());
	for (int i = 0; i < lightPool.size(); i++) {
		std::vector<VkDescriptorSetLayout> layoutsShadow(MAX_FRAMES_IN_FLIGHT * transformsSize, descriptorSetLayouts[i]);
		VkDescriptorSetAllocateInfo allocateInfoShadow{};
		allocateInfoShadow.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfoShadow.descriptorPool = descriptorPoolShadows[i];
		allocateInfoShadow.descriptorSetCount = MAX_FRAMES_IN_FLIGHT * transformsSize;
		allocateInfoShadow.pSetLayouts = layoutsShadow.data();
		descriptorSetsShadows[i].resize(MAX_FRAMES_IN_FLIGHT * transformsSize);
		if (vkAllocateDescriptorSets(device, &allocateInfoShadow, descriptorSetsShadows[i].data())
			!= VK_SUCCESS) {
			throw std::runtime_error("ERROR: Unable to create descriptor sets in Vulkan System. Shadow.");
		}
	}

	size_t samplerSize = rawTextures.size() + rawCubes.size();

	size_t pool = 0;
	for (; pool < transformPools.size() && useVertexBuffer; pool++) {
		for (int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
			size_t poolInd = pool * MAX_FRAMES_IN_FLIGHT + frame;
			std::vector<VkWriteDescriptorSet> shadowWriteDescriptorSets =
				std::vector<VkWriteDescriptorSet>(lightPool.size());

			for (int i = 0; i < lightPool.size(); i++) {
				//Shadow
				VkDescriptorBufferInfo bufferInfoModels{};
				bufferInfoModels.buffer = uniformBuffersModelsPools[i][pool][frame];
				bufferInfoModels.offset = 0;
				bufferInfoModels.range = sizeof(mat44<float>) * transformPools[pool].size();

				shadowWriteDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				shadowWriteDescriptorSets[i].dstSet = descriptorSetsShadows[i][poolInd];
				shadowWriteDescriptorSets[i].dstBinding = 0;
				shadowWriteDescriptorSets[i].dstArrayElement = 0;
				shadowWriteDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				shadowWriteDescriptorSets[i].descriptorCount = 1;
				shadowWriteDescriptorSets[i].pBufferInfo = &bufferInfoModels;
				vkUpdateDescriptorSets(device, 1, &shadowWriteDescriptorSets[i], 0, nullptr);
			}

			//HDR
			VkDescriptorBufferInfo bufferInfoTransforms{};
			bufferInfoTransforms.buffer = uniformBuffersTransformsPools[pool][frame];
			bufferInfoTransforms.offset = 0;
			bufferInfoTransforms.range = sizeof(mat44<float>) * transformPools[pool].size();
			VkDescriptorBufferInfo bufferInfoCameras{};
			bufferInfoCameras.buffer = uniformBuffersCamerasPools[pool][frame];
			bufferInfoCameras.offset = 0;
			bufferInfoCameras.range = sizeof(mat44<float>);
			VkDescriptorBufferInfo bufferInfoLights{};
			bufferInfoLights.buffer = uniformBuffersLightsPools[pool][frame];
			bufferInfoLights.offset = 0;
			bufferInfoLights.range = sizeof(Light) * lightPool.size();
			VkDescriptorBufferInfo bufferInfoLightTransforms{};
			bufferInfoLightTransforms.buffer = uniformBuffersLightTransformsPools[pool][frame];
			bufferInfoLightTransforms.offset = 0;
			bufferInfoLightTransforms.range = sizeof(mat44<float>) * lightPool.size();
			VkDescriptorBufferInfo bufferInfoLightPerspective{};
			bufferInfoLightPerspective.buffer = uniformBuffersLightPerspectivePools[pool][frame];
			bufferInfoLightPerspective.offset = 0;
			bufferInfoLightPerspective.range = sizeof(mat44<float>) * lightPool.size();
			VkDescriptorBufferInfo bufferInfoMaterials{};
			bufferInfoMaterials.buffer = uniformBuffersMaterialsPools[pool][frame];
			bufferInfoMaterials.offset = 0;
			bufferInfoMaterials.range = sizeof(DrawMaterial) * materialPools[pool].size();
			VkDescriptorBufferInfo bufferInfoNormTransforms{};
			VkDescriptorBufferInfo bufferInfoEnvTransforms{};
			if (rawEnvironment.has_value()) {
				bufferInfoNormTransforms.buffer = uniformBuffersNormalTransformsPools[pool][frame];
				bufferInfoNormTransforms.offset = 0;
				bufferInfoNormTransforms.range = sizeof(mat44<float>) * transformNormalPools[pool].size();
				bufferInfoEnvTransforms.buffer = uniformBuffersEnvironmentTransformsPools[pool][frame];
				bufferInfoEnvTransforms.offset = 0;
				bufferInfoEnvTransforms.range = sizeof(mat44<float>) * transformEnvironmentPools[pool].size();
			}
			std::vector<VkWriteDescriptorSet> writeDescriptorSets = rawEnvironment.has_value() ?
				std::vector<VkWriteDescriptorSet>(10 + samplerSize) :
				std::vector<VkWriteDescriptorSet>(7 + samplerSize);
			writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[0].dstSet = descriptorSetsHDR[poolInd];
			writeDescriptorSets[0].dstBinding = 0;
			writeDescriptorSets[0].dstArrayElement = 0;
			writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSets[0].descriptorCount = 1;
			writeDescriptorSets[0].pBufferInfo = &bufferInfoTransforms;
			writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[1].dstSet = descriptorSetsHDR[poolInd];
			writeDescriptorSets[1].dstBinding = 1;
			writeDescriptorSets[1].dstArrayElement = 0;
			writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSets[1].descriptorCount = 1;
			writeDescriptorSets[1].pBufferInfo = &bufferInfoCameras;
			writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[2].dstSet = descriptorSetsHDR[poolInd];
			writeDescriptorSets[2].dstBinding = 2;
			writeDescriptorSets[2].dstArrayElement = 0;
			writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSets[2].descriptorCount = 1;
			writeDescriptorSets[2].pBufferInfo = &bufferInfoMaterials;
			writeDescriptorSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[3].dstSet = descriptorSetsHDR[poolInd];
			writeDescriptorSets[3].dstBinding = 6;
			writeDescriptorSets[3].dstArrayElement = 0;
			writeDescriptorSets[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSets[3].descriptorCount = 1;
			writeDescriptorSets[3].pBufferInfo = &bufferInfoLightTransforms;
			writeDescriptorSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[4].dstSet = descriptorSetsHDR[poolInd];
			writeDescriptorSets[4].dstBinding = 7;
			writeDescriptorSets[4].dstArrayElement = 0;
			writeDescriptorSets[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSets[4].descriptorCount = 1;
			writeDescriptorSets[4].pBufferInfo = &bufferInfoLights;

			VkDescriptorImageInfo imageInfoLUT{};
			imageInfoLUT.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfoLUT.imageView = LUTImageView;
			imageInfoLUT.sampler = LUTSampler;
			writeDescriptorSets[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[5].dstSet = descriptorSetsHDR[poolInd];
			writeDescriptorSets[5].dstBinding = 5;
			writeDescriptorSets[5].dstArrayElement = 0;
			writeDescriptorSets[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSets[5].descriptorCount = 1;
			writeDescriptorSets[5].pImageInfo = &imageInfoLUT;


			writeDescriptorSets[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[6].dstSet = descriptorSetsHDR[poolInd];
			writeDescriptorSets[6].dstBinding = 9;
			writeDescriptorSets[6].dstArrayElement = 0;
			writeDescriptorSets[6].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSets[6].descriptorCount = 1;
			writeDescriptorSets[6].pBufferInfo = &bufferInfoLightPerspective;

			if (rawEnvironment.has_value()) {
				writeDescriptorSets[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[7].dstSet = descriptorSetsHDR[poolInd];
				writeDescriptorSets[7].dstBinding = 11;
				writeDescriptorSets[7].dstArrayElement = 0;
				writeDescriptorSets[7].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writeDescriptorSets[7].descriptorCount = 1;
				writeDescriptorSets[7].pBufferInfo = &bufferInfoNormTransforms;
				writeDescriptorSets[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[8].dstSet = descriptorSetsHDR[poolInd];
				writeDescriptorSets[8].dstBinding = 12;
				writeDescriptorSets[8].dstArrayElement = 0;
				writeDescriptorSets[8].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writeDescriptorSets[8].descriptorCount = 1;
				writeDescriptorSets[8].pBufferInfo = &bufferInfoEnvTransforms;
			}

			std::vector<VkDescriptorImageInfo> imageInfosTex = std::vector<VkDescriptorImageInfo>(rawTextures.size());
			for (size_t tex = 0; tex < rawTextures.size(); tex++) {
				size_t descSet = rawEnvironment.has_value() ? tex + 9 : tex + 7;
				imageInfosTex[tex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfosTex[tex].imageView = textureImageViews[tex];
				imageInfosTex[tex].sampler = textureSamplers[tex];
				writeDescriptorSets[descSet].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[descSet].dstSet = descriptorSetsHDR[poolInd];
				writeDescriptorSets[descSet].dstBinding = 3;
				writeDescriptorSets[descSet].dstArrayElement = tex;
				writeDescriptorSets[descSet].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptorSets[descSet].descriptorCount = 1;
				writeDescriptorSets[descSet].pImageInfo = &imageInfosTex[tex];
			}
			std::vector<VkDescriptorImageInfo> imageInfosCube = std::vector<VkDescriptorImageInfo>(rawCubes.size());
			for (size_t cube = 0; cube < rawCubes.size(); cube++) {
				size_t descSet = rawEnvironment.has_value() ? cube + 9 : cube + 7;
				descSet += rawTextures.size();
				imageInfosCube[cube].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfosCube[cube].imageView = cubeImageViews[cube];
				imageInfosCube[cube].sampler = cubeSamplers[cube];
				writeDescriptorSets[descSet].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[descSet].dstSet = descriptorSetsHDR[poolInd];
				writeDescriptorSets[descSet].dstBinding = 4;
				writeDescriptorSets[descSet].dstArrayElement = cube;
				writeDescriptorSets[descSet].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptorSets[descSet].descriptorCount = 1;
				writeDescriptorSets[descSet].pImageInfo = &imageInfosCube[cube];
			}

			if (rawEnvironment.has_value()) {
				VkDescriptorImageInfo imageInfoEnv;
				size_t descSet =samplerSize + 9;
				imageInfoEnv.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfoEnv.imageView = environmentImageView;
				imageInfoEnv.sampler = environmentSampler;
				writeDescriptorSets[descSet].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[descSet].dstSet = descriptorSetsHDR[poolInd];
				writeDescriptorSets[descSet].dstBinding = 10;
				writeDescriptorSets[descSet].dstArrayElement = 0;
				writeDescriptorSets[descSet].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptorSets[descSet].descriptorCount = 1;
				writeDescriptorSets[descSet].pImageInfo = &imageInfoEnv;
			}

			vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
		}
	}
	for (; useInstancing && pool < transformPools.size() + transformInstPoolsStore.size(); pool++) {
		for (int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
			size_t poolInd = pool * MAX_FRAMES_IN_FLIGHT + frame;



			for (int i = 0; i < lightPool.size(); i++) {

				//Shadow
				VkDescriptorBufferInfo bufferInfoModels{};
				bufferInfoModels.buffer = uniformBuffersModelsPools[i][pool][frame];
				bufferInfoModels.offset = 0;
				bufferInfoModels.range = sizeof(mat44<float>) * transformInstPoolsStore[pool - transformPools.size()].size();

				VkWriteDescriptorSet shadowWriteDescriptorSet{};
				shadowWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				shadowWriteDescriptorSet.dstSet = descriptorSetsShadows[i][poolInd];
				shadowWriteDescriptorSet.dstBinding = 0;
				shadowWriteDescriptorSet.dstArrayElement = 0;
				shadowWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				shadowWriteDescriptorSet.descriptorCount = 1;
				shadowWriteDescriptorSet.pBufferInfo = &bufferInfoModels;
				vkUpdateDescriptorSets(device, 1, &shadowWriteDescriptorSet, 0, nullptr);
			}


			VkDescriptorBufferInfo bufferInfoLightPerspective{};
			bufferInfoLightPerspective.buffer = uniformBuffersLightPerspectivePools[pool][frame];
			bufferInfoLightPerspective.offset = 0;
			bufferInfoLightPerspective.range = sizeof(mat44<float>) * lightPool.size();
			VkDescriptorBufferInfo bufferInfoTransforms{};
			bufferInfoTransforms.buffer = uniformBuffersTransformsPools[pool][frame];
			bufferInfoTransforms.offset = 0;
			bufferInfoTransforms.range = sizeof(mat44<float>) * transformInstPoolsStore[pool - transformPools.size()].size();
			VkDescriptorBufferInfo bufferInfoCameras{};
			bufferInfoCameras.buffer = uniformBuffersCamerasPools[pool][frame];
			bufferInfoCameras.offset = 0;
			bufferInfoCameras.range = sizeof(mat44<float>);
			VkDescriptorBufferInfo bufferInfoLights{};
			bufferInfoLights.buffer = uniformBuffersLightsPools[pool][frame];
			bufferInfoLights.offset = 0;
			bufferInfoLights.range = sizeof(Light) * lightPool.size();
			VkDescriptorBufferInfo bufferInfoLightTransforms{};
			bufferInfoLightTransforms.buffer = uniformBuffersLightTransformsPools[pool][frame];
			bufferInfoLightTransforms.offset = 0;
			bufferInfoLightTransforms.range = sizeof(mat44<float>) * lightPool.size();
			VkDescriptorBufferInfo bufferInfoMaterials{};
			bufferInfoMaterials.buffer = uniformBuffersMaterialsPools[pool][frame];
			bufferInfoMaterials.offset = 0;
			bufferInfoMaterials.range = sizeof(DrawMaterial);
			VkDescriptorBufferInfo bufferInfoNormTransforms{};
			VkDescriptorBufferInfo bufferInfoEnvTransforms{};
			if (rawEnvironment.has_value()) {
				bufferInfoNormTransforms.buffer = uniformBuffersNormalTransformsPools[pool][frame];
				bufferInfoNormTransforms.offset = 0;
				bufferInfoNormTransforms.range = sizeof(mat44<float>) * transformNormalPools[pool].size();
				bufferInfoEnvTransforms.buffer = uniformBuffersEnvironmentTransformsPools[pool][frame];
				bufferInfoEnvTransforms.offset = 0;
				bufferInfoEnvTransforms.range = sizeof(mat44<float>) * transformEnvironmentInstPoolsStore[pool - transformPools.size()].size();
			}
			std::vector<VkWriteDescriptorSet> writeDescriptorSets = rawEnvironment.has_value() ?
				std::vector<VkWriteDescriptorSet>(10 + samplerSize) :
				std::vector<VkWriteDescriptorSet>(7 + samplerSize);

			writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[0].dstSet = descriptorSetsHDR[poolInd];
			writeDescriptorSets[0].dstBinding = 0;
			writeDescriptorSets[0].dstArrayElement = 0;
			writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSets[0].descriptorCount = 1;
			writeDescriptorSets[0].pBufferInfo = &bufferInfoTransforms;
			writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[1].dstSet = descriptorSetsHDR[poolInd];
			writeDescriptorSets[1].dstBinding = 1;
			writeDescriptorSets[1].dstArrayElement = 0;
			writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSets[1].descriptorCount = 1;
			writeDescriptorSets[1].pBufferInfo = &bufferInfoCameras;
			writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[2].dstSet = descriptorSetsHDR[poolInd];
			writeDescriptorSets[2].dstBinding = 2;
			writeDescriptorSets[2].dstArrayElement = 0;
			writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSets[2].descriptorCount = 1;
			writeDescriptorSets[2].pBufferInfo = &bufferInfoMaterials;
			writeDescriptorSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[3].dstSet = descriptorSetsHDR[poolInd];
			writeDescriptorSets[3].dstBinding = 6;
			writeDescriptorSets[3].dstArrayElement = 0;
			writeDescriptorSets[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSets[3].descriptorCount = 1;
			writeDescriptorSets[3].pBufferInfo = &bufferInfoLightTransforms;
			writeDescriptorSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[4].dstSet = descriptorSetsHDR[poolInd];
			writeDescriptorSets[4].dstBinding = 7;
			writeDescriptorSets[4].dstArrayElement = 0;
			writeDescriptorSets[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSets[4].descriptorCount = 1;
			writeDescriptorSets[4].pBufferInfo = &bufferInfoLights;

			VkDescriptorImageInfo imageInfoLUT{};
			imageInfoLUT.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfoLUT.imageView = LUTImageView;
			imageInfoLUT.sampler = LUTSampler;
			writeDescriptorSets[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[5].dstSet = descriptorSetsHDR[poolInd];
			writeDescriptorSets[5].dstBinding = 5;
			writeDescriptorSets[5].dstArrayElement = 0;
			writeDescriptorSets[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSets[5].descriptorCount = 1;
			writeDescriptorSets[5].pImageInfo = &imageInfoLUT;

			writeDescriptorSets[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[6].dstSet = descriptorSetsHDR[poolInd];
			writeDescriptorSets[6].dstBinding = 9;
			writeDescriptorSets[6].dstArrayElement = 0;
			writeDescriptorSets[6].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSets[6].descriptorCount = 1;
			writeDescriptorSets[6].pBufferInfo = &bufferInfoLightPerspective;

			if (rawEnvironment.has_value()) {
				writeDescriptorSets[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[7].dstSet = descriptorSetsHDR[poolInd];
				writeDescriptorSets[7].dstBinding = 11;
				writeDescriptorSets[7].dstArrayElement = 0;
				writeDescriptorSets[7].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writeDescriptorSets[7].descriptorCount = 1;
				writeDescriptorSets[7].pBufferInfo = &bufferInfoNormTransforms;
				writeDescriptorSets[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[8].dstSet = descriptorSetsHDR[poolInd];
				writeDescriptorSets[8].dstBinding = 12;
				writeDescriptorSets[8].dstArrayElement = 0;
				writeDescriptorSets[8].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writeDescriptorSets[8].descriptorCount = 1;
				writeDescriptorSets[8].pBufferInfo = &bufferInfoEnvTransforms;
			}

			std::vector<VkDescriptorImageInfo> imageInfosTex = std::vector<VkDescriptorImageInfo>(rawTextures.size());
			for (size_t tex = 0; tex < rawTextures.size(); tex++) {
				size_t descSet = rawEnvironment.has_value() ? tex + 9 : tex + 7;
				imageInfosTex[tex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfosTex[tex].imageView = textureImageViews[tex];
				imageInfosTex[tex].sampler = textureSamplers[tex];
				writeDescriptorSets[descSet].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[descSet].dstSet = descriptorSetsHDR[poolInd];
				writeDescriptorSets[descSet].dstBinding = 3;
				writeDescriptorSets[descSet].dstArrayElement = tex;
				writeDescriptorSets[descSet].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptorSets[descSet].descriptorCount = 1;
				writeDescriptorSets[descSet].pImageInfo = &imageInfosTex[tex];
			}

			std::vector<VkDescriptorImageInfo> imageInfosCube = std::vector<VkDescriptorImageInfo>(rawCubes.size());
			for (size_t cube = 0; cube < rawCubes.size(); cube++) {
				size_t descSet = rawEnvironment.has_value() ? cube + 9 : cube + 7;
				descSet += rawTextures.size();
				imageInfosCube[cube].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfosCube[cube].imageView = cubeImageViews[cube];
				imageInfosCube[cube].sampler = cubeSamplers[cube];
				writeDescriptorSets[descSet].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[descSet].dstSet = descriptorSetsHDR[poolInd];
				writeDescriptorSets[descSet].dstBinding = 4;
				writeDescriptorSets[descSet].dstArrayElement = cube;
				writeDescriptorSets[descSet].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptorSets[descSet].descriptorCount = 1;
				writeDescriptorSets[descSet].pImageInfo = &imageInfosCube[cube];
			}



			if (rawEnvironment.has_value()) {
				VkDescriptorImageInfo imageInfoEnv;
				size_t descSet = samplerSize + 9;
				imageInfoEnv.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfoEnv.imageView = environmentImageView;
				imageInfoEnv.sampler = environmentSampler;
				writeDescriptorSets[descSet].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[descSet].dstSet = descriptorSetsHDR[poolInd];
				writeDescriptorSets[descSet].dstBinding = 10;
				writeDescriptorSets[descSet].dstArrayElement = 0;
				writeDescriptorSets[descSet].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptorSets[descSet].descriptorCount = 1;
				writeDescriptorSets[descSet].pImageInfo = &imageInfoEnv;
			}
			vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
		}
	}

	for (size_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
		VkDescriptorImageInfo finalDescriptor{};
		finalDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		finalDescriptor.imageView = attachmentImageViews[frame];
		finalDescriptor.sampler = VK_NULL_HANDLE;

		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = descriptorSetsFinal[frame];
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.dstBinding = 0;
		writeDescriptorSet.pImageInfo = &finalDescriptor;
		writeDescriptorSet.dstArrayElement = 0;
		vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
	}
}

void VulkanSystem::createCommands() {
	commandBuffers.resize((1 + lightPool.size()) * MAX_FRAMES_IN_FLIGHT);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = familyIndices.graphicsFamily.value();
	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("ERROR: Unable to create a command pool in VulkanSystem.");
	}

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = commandBuffers.size();
	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("ERROR: Unable to create a command buffer in VulkanSystem.");
	}



}

void VulkanSystem::createImage(uint32_t width, uint32_t height, VkFormat format, 
	VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkMemoryPropertyFlags properties, 
	VkImage& image, VkDeviceMemory& imageMemory, int layers, int levels) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = levels;
	imageInfo.arrayLayers = layers;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.flags = flags;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("ERROR: Unable to create an image in VulkanSystem.");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties,physicalDevice);
	if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("ERROR: Unable to allocate an image memory in Vulkan System!");
	}
	vkBindImageMemory(device, image, imageMemory, 0);
}

void VulkanSystem::createDepthResources() {
	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
	createImage(swapChainExtent.width, swapChainExtent.height, depthFormat,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 0,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
	depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	VkFormat shadowDepthFormat = VK_FORMAT_D16_UNORM;
	shadowDepthImages.resize(lightPool.size());
	shadowDepthImageViews.resize(lightPool.size());
	shadowDepthImageMemorys.resize(lightPool.size());
	for (int i = 0; i < lightPool.size(); i++) {
		size_t shadowRes = lightPool[i].shadowRes;
		if (shadowRes == 0) shadowRes = 1;
		createImage(shadowRes, shadowRes, shadowDepthFormat,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shadowDepthImages[i], shadowDepthImageMemorys[i]);
		shadowDepthImageViews[i] = createImageView(shadowDepthImages[i], shadowDepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	}
}

void VulkanSystem::recordCommandBufferShadow(VkCommandBuffer commandBuffer, uint32_t imageIndex, int lightIndex) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("ERROR: Unable to begin recording a command buffer in VulkanSystem.");
	}



	VkExtent2D shadowExtent;
	size_t shadowRes = lightPool[lightIndex].shadowRes;
	if (shadowRes == 0) shadowRes = 1;
	shadowExtent.width = shadowRes;
	shadowExtent.height = shadowRes;


	//Begin preparing command buffer render pass
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = shadowPasses[lightIndex];
	renderPassInfo.framebuffer = shadowFramebuffers[lightIndex][imageIndex];
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent = shadowExtent;
	std::vector<VkClearValue> clearColors;
	clearColors.push_back({ {1.0f,1.0,1.0,0} });
	clearColors.push_back({ {1.0f,0} });

	renderPassInfo.clearValueCount = clearColors.size();
	renderPassInfo.pClearValues = clearColors.data();
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	VkRect2D scissor{};
	//Viewport and Scissor are dyanmic, so set now
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(shadowRes);
	viewport.height = static_cast<float>(shadowRes);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	scissor.offset = { 0, 0 };
	scissor.extent = shadowExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	//Shadow subpass;
	{
		vkCmdPushConstants(commandBuffer, pipelineLayoutShadows[lightIndex], VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat44<float>), &worldTolightPerspPool[lightIndex]);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineShadows[lightIndex]);
		const VkDeviceSize offsets[] = { 0 };
		if (useVertexBuffer) vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
		for (size_t pool = 0; pool < transformPools.size() && pool < indexBuffersValid.size() && useVertexBuffer; pool++) {
			if (indexBuffersValid[pool]) {
				vkCmdBindIndexBuffer(commandBuffer, indexBuffers[pool], 0, VK_INDEX_TYPE_UINT32);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelineLayoutShadows[lightIndex], 0, 1, &descriptorSetsShadows[lightIndex][pool * MAX_FRAMES_IN_FLIGHT + currentFrame], 0, nullptr);
				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indexPools[pool].size()), 1, 0, 0, 0);
			}
		}

	}
	//Instanced version
	if (useInstancing) {
		vkCmdPushConstants(commandBuffer, pipelineLayoutShadows[lightIndex], VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat44<float>), &worldTolightPerspPool[lightIndex]);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsInstPipelineShadows[lightIndex]);
		const VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexInstBuffer, offsets);
		for (size_t pool = 0; pool < transformInstPools.size(); pool++) {
			if (transformInstPools[pool].size() == 0) continue;
			vkCmdBindIndexBuffer(commandBuffer, indexInstBuffers[transformInstIndexPools[pool]], 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayoutShadows[lightIndex], 0, 1, &descriptorSetsShadows[lightIndex][(pool + transformPools.size()) *
				MAX_FRAMES_IN_FLIGHT + currentFrame], 0, nullptr);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(
				indexInstPools[transformInstIndexPools[pool]].size()),
				transformInstPools[pool].size(), 0, 0, 0);
		}

	}
	//END render pass
	vkCmdEndRenderPass(commandBuffer);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = shadowDepthImages[lightIndex];
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
		&barrier);



	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("ERROR: Unable to record command buffer in VulkanSystem.");
	}


}

void VulkanSystem::recordCommandBufferMain(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("ERROR: Unable to begin recording a command buffer in VulkanSystem.");
	}

	//Begin preparing command buffer render pass
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent = swapChainExtent;
	std::vector<VkClearValue> clearColors;
	clearColors.push_back({ {0.0f, 0.0f, 0.0f, 1.0f} });
	clearColors.push_back({ {1.0f,0} });
	clearColors.push_back({ {0.0f, 0.0f, 0.0f, 1.0f} });

	renderPassInfo.clearValueCount = clearColors.size();
	renderPassInfo.pClearValues = clearColors.data();


	VkViewport viewport{};
	VkRect2D scissor{};
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	//Viewport and Scissor are dyanmic, so set now
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChainExtent.width);
	viewport.height = static_cast<float>(swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	//Main subpass
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
		//Begin recording commands
		vkCmdPushConstants(commandBuffer, pipelineLayoutHDR, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConst), &pushConstHDR);
		const VkDeviceSize offsets[] = { 0 };
		if (useVertexBuffer) vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
		for (size_t pool = 0; pool < transformPools.size() && pool < indexBuffersValid.size() && useVertexBuffer; pool++) {
			if (indexBuffersValid[pool]) {
				vkCmdBindIndexBuffer(commandBuffer, indexBuffers[pool], 0, VK_INDEX_TYPE_UINT32);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelineLayoutHDR, 0, 1, &descriptorSetsHDR[pool * MAX_FRAMES_IN_FLIGHT + currentFrame], 0, nullptr);
				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indexPools[pool].size()), 1, 0, 0, 0);
			}
		}
	}

	//Instanced version
	if (useInstancing) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsInstPipeline);


		vkCmdPushConstants(commandBuffer, pipelineLayoutHDR, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConst), &pushConstHDR);

		const VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexInstBuffer, offsets);
		for (size_t pool = 0; pool < transformInstPools.size(); pool++) {
			if (transformInstPools[pool].size() == 0) continue;
			vkCmdBindIndexBuffer(commandBuffer, indexInstBuffers[transformInstIndexPools[pool]], 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayoutHDR, 0, 1, &descriptorSetsHDR[(pool + transformPools.size()) *
				MAX_FRAMES_IN_FLIGHT + currentFrame], 0, nullptr);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(
				indexInstPools[transformInstIndexPools[pool]].size()),
				transformInstPools[pool].size(), 0, 0, 0);
		}

	}

	//Present subpass
	vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineFinal);
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayoutFinal, 0, 1, &descriptorSetsFinal[currentFrame], 0, NULL);
	vkCmdDraw(commandBuffer, 6, 1, 0, 0); //Draw a quad in shader

	//END render pass
	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("ERROR: Unable to record command buffer in VulkanSystem.");
	}


}



void VulkanSystem::updateUniformBuffers(uint32_t frame) {


	cullInstances();
	//Guided by glm implementation of lookAt
	float_3 useMoveVec = movementMode == MOVE_DEBUG ? debugMoveVec : moveVec;
	float_3 useDirVec = movementMode == MOVE_DEBUG ? debugDirVec : dirVec;
	mat44<float> local = getCameraSpace(cameras[currentCamera], useMoveVec, useDirVec);
	float_3 cameraPos = useMoveVec + cameras[currentCamera].forAnimate.translate;
	local = cameras[currentCamera].perspective * local;
	pushConstHDR.numLights = (int)lightPool.size();
	pushConstHDR.camPosX = cameraPos.x;
	pushConstHDR.camPosY = cameraPos.y;
	pushConstHDR.camPosZ = cameraPos.z;
	pushConstHDR.pbrP = 3;
	size_t pool = 0;
	size_t matsize = sizeof(DrawMaterial);
	for (; pool < transformPools.size() && useVertexBuffer; pool++) {
		memcpy(uniformBuffersMappedTransformsPools[pool][frame],
			transformPools[pool].data(), sizeof(mat44<float>) *
			transformPools[pool].size());
		for (int i = 0; i < lightPool.size(); i++) {
			memcpy(uniformBuffersMappedModelsPools[i][pool][frame],
				transformPools[pool].data(), sizeof(mat44<float>) *
				transformPools[pool].size());
		}
		if (rawEnvironment.has_value()) {
			memcpy(uniformBuffersMappedNormalTransformsPools[pool][frame],
				transformNormalPools[pool].data(), sizeof(mat44<float>) *
				transformNormalPools[pool].size());
			memcpy(uniformBuffersMappedEnvironmentTransformsPools[pool][frame],
				transformEnvironmentPools[pool].data(), sizeof(mat44<float>) *
				transformEnvironmentPools[pool].size());
		}
		memcpy(uniformBuffersMappedCamerasPools[pool][frame], &(local),
			sizeof(mat44<float>));
		memcpy(uniformBuffersMappedLightsPools[pool][frame], lightPool.data(),
			sizeof(DrawLight) * lightPool.size());
		memcpy(uniformBuffersMappedLightTransformsPools[pool][frame], worldTolightPool.data(),
			sizeof(mat44<float>) * worldTolightPool.size());
		memcpy(uniformBuffersMappedLightPerspectivePools[pool][frame], worldTolightPerspPool.data(),
			sizeof(mat44<float>) * worldTolightPool.size());
		memcpy(uniformBuffersMappedMaterialsPools[pool][frame], 
			materialPools[pool].data(), matsize * materialPools[pool].size());
	}
	if (!useInstancing) return;
	for (; pool < transformPools.size() + transformInstPools.size(); pool++) {
		size_t poolAdjusted = pool - transformPools.size();
		if (transformInstPools[poolAdjusted].size() == 0) continue;
		memcpy(uniformBuffersMappedTransformsPools[pool][frame],
			transformInstPools[poolAdjusted].data(), sizeof(mat44<float>) *
			transformInstPools[poolAdjusted].size());
		for (int i = 0; i < lightPool.size(); i++) {
			memcpy(uniformBuffersMappedModelsPools[i][pool][frame],
				transformInstPools[poolAdjusted].data(), sizeof(mat44<float>) *
				transformInstPools[poolAdjusted].size());
		}
		if (rawEnvironment.has_value()) {
			memcpy(uniformBuffersMappedNormalTransformsPools[pool][frame],
				transformNormalInstPools[poolAdjusted].data(), sizeof(mat44<float>) *
				transformNormalInstPools[poolAdjusted].size());
			memcpy(uniformBuffersMappedEnvironmentTransformsPools[pool][frame],
				transformEnvironmentInstPools[poolAdjusted].data(), sizeof(mat44<float>) *
				transformEnvironmentInstPools[poolAdjusted].size());
		}
		memcpy(uniformBuffersMappedCamerasPools[pool][frame], &(local),
			sizeof(mat44<float>));
		memcpy(uniformBuffersMappedLightsPools[pool][frame], lightPool.data(),
			sizeof(DrawLight)* lightPool.size());
		memcpy(uniformBuffersMappedLightTransformsPools[pool][frame], worldTolightPool.data(),
			sizeof(mat44<float>) * worldTolightPool.size());
		memcpy(uniformBuffersMappedLightPerspectivePools[pool][frame], worldTolightPerspPool.data(),
			sizeof(mat44<float>) * worldTolightPool.size());
		memcpy(uniformBuffersMappedMaterialsPools[pool][frame],
			&instancedMaterials[poolAdjusted], matsize);
	}
}


void VulkanSystem::drawFrame() {



	VkResult result;

	uint32_t imageIndex;
	if (renderToWindow) {
		vkWaitForFences(device, 1, inFlightFences.data() + currentFrame, VK_TRUE, UINT64_MAX);
	
		if (!*activeP) { return; }
		result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS || !*activeP) {
			throw std::runtime_error("ERROR: Unable to acquire a swap chain image in VulkanSystem.");
		}

		vkResetFences(device, 1, inFlightFences.data() + currentFrame);

	}
	else { //Headless
		if (headlessGuard) return;
		headlessGuard = true;
		imageIndex = currentFrame % MAX_FRAMES_IN_FLIGHT;
	}



	createVertexBuffer(false);
	createIndexBuffers(true, true);
	updateUniformBuffers(currentFrame);


	for (int i = 0; i < lightPool.size(); i++) {

		size_t commandBufferIndex = (lightPool.size() + 1) * currentFrame + i;

		vkResetCommandBuffer(commandBuffers[commandBufferIndex], 0);
		recordCommandBufferShadow(commandBuffers[commandBufferIndex], imageIndex,i);



		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = commandBuffers.data() + commandBufferIndex;

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}


	}



	for (size_t pool = 0; pool < transformPools.size() + transformInstPoolsStore.size(); pool++) {
		std::vector<VkWriteDescriptorSet> writeDescriptorSets = std::vector<VkWriteDescriptorSet>(lightPool.size());
		std::vector<VkDescriptorImageInfo> imageInfoShadows = std::vector<VkDescriptorImageInfo>(lightPool.size());
		size_t poolInd = pool * MAX_FRAMES_IN_FLIGHT + currentFrame;

		for (int light = 0; light < lightPool.size(); light++) {
			bool useDefault = lightPool[light].shadowRes == 0;
			writeDescriptorSets[light] = {};
			imageInfoShadows[light] = {};
			imageInfoShadows[light].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfoShadows[light].imageView = useDefault ? defaultShadowImageView : shadowDepthImageViews[light];
			imageInfoShadows[light].sampler = useDefault ? defaultShadowSampler : shadowSamplers[light][currentFrame];
			writeDescriptorSets[light].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[light].dstSet = descriptorSetsHDR[poolInd];
			writeDescriptorSets[light].dstBinding = 8;
			writeDescriptorSets[light].dstArrayElement = light;
			writeDescriptorSets[light].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSets[light].descriptorCount = 1;
			writeDescriptorSets[light].pImageInfo = &imageInfoShadows[light];
		}
		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);

	}


	initialFrame = false;
	size_t commandBufferIndex = (lightPool.size() + 1) * currentFrame + lightPool.size();

	vkResetCommandBuffer(commandBuffers[commandBufferIndex], 0);
	recordCommandBufferMain(commandBuffers[commandBufferIndex], imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	if (renderToWindow) {

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;


		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = commandBuffers.data() + (commandBufferIndex);
		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &imageIndex;

		VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("ERROR: Unable to present a swap chain image in VulkanSystem.");
		}
	}
	else {
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = commandBuffers.data() + (commandBufferIndex);

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}
	}

	currentFrame++; currentFrame %= MAX_FRAMES_IN_FLIGHT;
	//Headless benchmarking
	if (!renderToWindow) {
		headlessFrames++;
		std::cout << "Frames rendered: " << headlessFrames << std::endl;
	}
}


void VulkanSystem::recreateSwapChain() {

	vkDeviceWaitIdle(device);
	handleWindowMinimized(mainWindow);
	if (*activeP)
	{
		cleanupSwapChain();
		createSwapChain();
		createImageViews();
		createDepthResources();
		createFramebuffers();
	}
}

void VulkanSystem::cleanupSwapChain() {
	for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
		vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
	}
	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		vkDestroyImageView(device, swapChainImageViews[i], nullptr);
	}
	vkDestroySwapchainKHR(device, swapChain, nullptr);
}