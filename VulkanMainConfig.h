#pragma once


struct VulkanMainConfig
{
	GLFWwindow* window;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
	uint32_t width = 0;
	uint32_t height = 0;
	VkPresentModeKHR* presentModes = nullptr;
	const char* objPath = nullptr;
	const char* texFilePath = nullptr;
	bool useSplitscreen = false;
	bool useWireframe = false;
	bool useCelShading = false;
};