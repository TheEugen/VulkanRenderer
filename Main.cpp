#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>

#include "VulkanBase.h"
#include "VulkanMain.h"
#include "VulkanMainConfig.h"

auto procStartTime = std::chrono::high_resolution_clock::now();

void eventLoop(GLFWwindow* window, VulkanMain& vulkanMain)
{
	auto endTime = std::chrono::high_resolution_clock::now();
	float timeSinceStart = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - procStartTime).count() / 1000.0f;
	std::cout << "program start time: " << timeSinceStart << "s" << std::endl;

	VkDevice device = vulkanMain.getDevice();

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		
		vulkanMain.updateMVP();	
		vulkanMain.drawFrame();
		vkDeviceWaitIdle(device);
	}
}

int main()
{	
	const char* objPath = "meshes//dragon.obj"; // chalet.obj
	const char* texPath = "images//skull_diffuse.jpg";

	VulkanBase vulkanBase;
	vulkanBase.init();
	
	VulkanMainConfig vulkanMainConfig;
	vulkanMainConfig.window = vulkanBase.getWindow();
	vulkanMainConfig.physicalDevice = vulkanBase.getPhysicalDevice();
	vulkanMainConfig.surface = vulkanBase.getSurface();
	vulkanMainConfig.surfaceCapabilities = vulkanBase.getStoredCapabilities();
	vulkanMainConfig.width = vulkanBase.getWidth();
	vulkanMainConfig.height = vulkanBase.getHeight();
	vulkanMainConfig.presentModes = vulkanBase.getPresentModes();
	vulkanMainConfig.objPath = objPath;
	vulkanMainConfig.texFilePath = texPath;
	vulkanMainConfig.useSplitscreen = false;
	vulkanMainConfig.useWireframe = false;
	vulkanMainConfig.useCelShading = false;
	
	
	VulkanMain vulkanMain { vulkanMainConfig };

	vulkanBase.setVulkanMain(vulkanMain);

	vulkanMain.init();
	
	eventLoop(vulkanBase.getWindow(), vulkanMain);


	return 0;
}