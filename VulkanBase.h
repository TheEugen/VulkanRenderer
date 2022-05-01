#pragma once

#include <iostream>
#include <vector>

#include "VulkanMain.h"

#define VK_CHECK(val) if(val != VK_SUCCESS) __debugbreak();

class VulkanBase
{
public:
	

	VulkanBase() {}

	~VulkanBase()
	{
		destroy();
	}

	void init()
	{
		if (m_initialized)
			throw std::runtime_error("VulkanBase already initialized");

		initGlfw();
		createInstance();
		createSurface();
		getPhysicalDevices();
		choosePhysicalDevice();
		getCapabilities();
		printStats();

		m_initialized = true;
	}

	void destroy()
	{
		checkIfInitialized();

		delete[] m_surfaceFormats;
		delete[] m_presentModes;

		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		vkDestroyInstance(m_instance, nullptr);

		glfwDestroyWindow(m_window);
		glfwTerminate();
	}
	
	void setVulkanMain(VulkanMain& vulkanMain)
	{
		m_vulkanMain = &vulkanMain;
	}

	void getCapabilities()
	{
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_chosenPhysicalDevice, m_surface, &m_surfaceCapabilities);

		vkGetPhysicalDeviceSurfaceFormatsKHR(m_chosenPhysicalDevice, m_surface, &m_surfaceFormatAmount, nullptr);
		m_surfaceFormats = new VkSurfaceFormatKHR[m_surfaceFormatAmount];
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_chosenPhysicalDevice, m_surface, &m_surfaceFormatAmount, m_surfaceFormats);

		vkGetPhysicalDeviceSurfacePresentModesKHR(m_chosenPhysicalDevice, m_surface, &m_presentModeAmount, nullptr);
		m_presentModes = new VkPresentModeKHR[m_presentModeAmount];
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_chosenPhysicalDevice, m_surface, &m_presentModeAmount, m_presentModes);
	}

	uint32_t getWidth()
	{
		checkIfInitialized();
		return m_width;
	}

	uint32_t getHeight()
	{
		checkIfInitialized();
		return m_height;
	}

	VkInstance getInstance()
	{
		checkIfInitialized();
		return m_instance;
	}

	VkSurfaceKHR getSurface()
	{
		checkIfInitialized();
		return m_surface;
	}

	VkPhysicalDevice getPhysicalDevice()
	{
		checkIfInitialized();
		return m_chosenPhysicalDevice;
	}

	VkSurfaceCapabilitiesKHR getStoredCapabilities()
	{
		checkIfInitialized();
		return m_surfaceCapabilities;
	}

	VkPresentModeKHR* getPresentModes()
	{
		checkIfInitialized();
		return m_presentModes;
	}

	GLFWwindow* getWindow()
	{
		checkIfInitialized();
		return m_window;
	}


	
private:
	static VulkanMain* m_vulkanMain;

	GLFWwindow* m_window = NULL;
	VkInstance m_instance = VK_NULL_HANDLE;
	uint32_t m_width = 800;
	uint32_t m_height = 600;
	VkSurfaceKHR m_surface = 0;
	std::vector<VkPhysicalDevice> m_physicalDevices = {};
	VkPhysicalDevice m_chosenPhysicalDevice = VK_NULL_HANDLE;
	VkSurfaceCapabilitiesKHR m_surfaceCapabilities = {};
	uint32_t m_surfaceFormatAmount = 0;
	VkSurfaceFormatKHR* m_surfaceFormats = nullptr;
	uint32_t m_presentModeAmount = 0;
	VkPresentModeKHR* m_presentModes = nullptr;

	const std::vector<const char*> m_validationLayers =
	{
		"VK_LAYER_LUNARG_standard_validation"
	};

	const char* m_appName = "Vulkan_Test_1";
	const char* m_windowTitle = "Renderer Test";

	bool m_initialized = false;


	static void onWindowResized(GLFWwindow* window, int width, int height)
	{		
		if (width == 0 || height == 0)
			return;

		m_vulkanMain->setWidth(width);
		m_vulkanMain->setHeight(height);
		m_vulkanMain->recreateSwapchain();
		m_vulkanMain->drawFrame();
	}

	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (action == GLFW_PRESS || action == GLFW_REPEAT)
			m_vulkanMain->onMoving(key);
	}

	void initGlfw()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_window = glfwCreateWindow(m_width, m_height, m_windowTitle, nullptr, nullptr);
		glfwSetWindowUserPointer(m_window, this);
		glfwSetWindowSizeCallback(m_window, onWindowResized);

		glfwSetKeyCallback(m_window, keyCallback);
		glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GLFW_TRUE);
	}

	void createInstance()
	{
		VkApplicationInfo appInfo;
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pNext = nullptr;
		appInfo.pApplicationName = m_appName;
		appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
		appInfo.pEngineName = "Test Engine Name";
		appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_1;

		uint32_t layersAmount = 0;
		VkResult result = vkEnumerateInstanceLayerProperties(&layersAmount, nullptr);
		VK_CHECK(result);
		VkLayerProperties* layers = new VkLayerProperties[layersAmount];
		result = vkEnumerateInstanceLayerProperties(&layersAmount, layers);
		VK_CHECK(result);

		std::cout << "InstanceLayer Amount: " << layersAmount << std::endl;
		for (uint32_t i = 0; i < layersAmount; ++i)
			std::cout << "InstanceLayer #" << i << " Name: " << layers[i].layerName << std::endl;
		std::cout << std::endl;

		uint32_t extensionsAmount = 0;
		result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsAmount, nullptr);
		VkExtensionProperties* extensions = new VkExtensionProperties[extensionsAmount];
		result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsAmount, extensions);

		std::cout << "Extension Amount: " << extensionsAmount << std::endl;
		for (uint32_t i = 0; i < extensionsAmount; ++i)
			std::cout << "Extension #" << i << " Name: " << extensions[i].extensionName << std::endl;
		std::cout << std::endl;

		uint32_t glfwExtensionsAmount = 0;
		auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsAmount);

		VkInstanceCreateInfo instanceCreateInfo;
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pNext = nullptr;
		instanceCreateInfo.flags = 0;
		instanceCreateInfo.pApplicationInfo = &appInfo;
		instanceCreateInfo.enabledLayerCount = m_validationLayers.size();
		instanceCreateInfo.ppEnabledLayerNames = m_validationLayers.data();
		instanceCreateInfo.enabledExtensionCount = glfwExtensionsAmount;
		instanceCreateInfo.ppEnabledExtensionNames = glfwExtensions;

		result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance);
		VK_CHECK(result);

		delete[] extensions;
		delete[] layers;
	}

	void createSurface()
	{
		VkResult result = glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface);
		VK_CHECK(result);
	}

	void getPhysicalDevices()
	{
		uint32_t physicalDevicesAmount = 0;
		VkResult result = vkEnumeratePhysicalDevices(m_instance, &physicalDevicesAmount, nullptr);
		VK_CHECK(result);

		if (physicalDevicesAmount == 0)
			throw std::runtime_error("no vulkan capable device found");

		m_physicalDevices.resize(physicalDevicesAmount);
		result = vkEnumeratePhysicalDevices(m_instance, &physicalDevicesAmount, m_physicalDevices.data());
		VK_CHECK(result);
	}

	void choosePhysicalDevice()
	{
		m_chosenPhysicalDevice = m_physicalDevices[0];
	}

	void printStats()
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(m_chosenPhysicalDevice, &properties);

		uint32_t apiVer = properties.apiVersion;
		std::cout << "API Version:			" << VK_VERSION_MAJOR(apiVer) << "." << VK_VERSION_MINOR(apiVer) << "." << VK_VERSION_PATCH(apiVer) << std::endl;

		uint32_t driverVer = properties.driverVersion;
		std::cout << "Driver Version:			" << VK_VERSION_MAJOR(driverVer) << "." << VK_VERSION_MINOR(driverVer) << "." << VK_VERSION_PATCH(driverVer) << std::endl;
		std::cout << "Vendor ID:			" << properties.vendorID << std::endl;
		std::cout << "Device ID:			" << properties.deviceID << std::endl;
		std::cout << "Device Type:			" << properties.deviceType << std::endl;
		std::cout << "Name:				" << properties.deviceName << std::endl;
		std::cout << "Queue Priority Limit:		" << properties.limits.discreteQueuePriorities << "\n" << std::endl;

		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(m_chosenPhysicalDevice, &features);
		std::cout << "PhysicalDeviceFeatures" << std::endl;
		std::cout << "Geometry Shader:		" << features.geometryShader << "\n" << std::endl;

		VkPhysicalDeviceMemoryProperties memProps;
		vkGetPhysicalDeviceMemoryProperties(m_chosenPhysicalDevice, &memProps);

		uint32_t queueFamiliesAmount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(m_chosenPhysicalDevice, &queueFamiliesAmount, nullptr);
		VkQueueFamilyProperties* familyProps = new VkQueueFamilyProperties[queueFamiliesAmount];
		vkGetPhysicalDeviceQueueFamilyProperties(m_chosenPhysicalDevice, &queueFamiliesAmount, familyProps);

		for (uint32_t i = 0; i < queueFamiliesAmount; ++i)
		{
			std::cout << "Queue Family #" << i << std::endl;
			std::cout << "GraphicsBit " << ((familyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) << " ";
			std::cout << "ComputeBit " << ((familyProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) << " ";
			std::cout << "TransferBit " << ((familyProps[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) << " ";
			std::cout << "SparseBindingBit " << ((familyProps[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) != 0) << " ";
			std::cout << "ProtectedBit " << ((familyProps[i].queueFlags & VK_QUEUE_PROTECTED_BIT) != 0) << std::endl;
			std::cout << "Queue count: " << familyProps[i].queueCount << std::endl;
			std::cout << "TimestampValidBits: " << familyProps[i].timestampValidBits << "\n" << std::endl;
		}

		std::cout << "Surface Capabilities" << std::endl;
		std::cout << "minImageCount " << m_surfaceCapabilities.minImageCount << std::endl;
		std::cout << "maxImageCount " << m_surfaceCapabilities.maxImageCount << std::endl;
		std::cout << "currentExtent " << m_surfaceCapabilities.currentExtent.width << "/" << m_surfaceCapabilities.currentExtent.height << std::endl;
		std::cout << "minImageExtent " << m_surfaceCapabilities.minImageExtent.width << "/" << m_surfaceCapabilities.minImageExtent.height << std::endl;
		std::cout << "maxImageExtent " << m_surfaceCapabilities.maxImageExtent.width << "/" << m_surfaceCapabilities.maxImageExtent.height << std::endl;
		std::cout << "maxImageArrayLayers " << m_surfaceCapabilities.maxImageArrayLayers << std::endl;
		std::cout << "supportedTransforms " << m_surfaceCapabilities.supportedTransforms << std::endl;
		std::cout << "currentTransform " << m_surfaceCapabilities.currentTransform << std::endl;
		std::cout << "supportedCompositeAlpha " << m_surfaceCapabilities.supportedCompositeAlpha << std::endl;
		std::cout << "supportedUsageFlags " << m_surfaceCapabilities.supportedUsageFlags << "\n" << std::endl;

		std::cout << "Format Amount: " << m_surfaceFormatAmount << std::endl;
		for (uint32_t i = 0; i < m_surfaceFormatAmount; ++i)
			std::cout << "Format #" << i << " " << m_surfaceFormats[i].format << std::endl;
		std::cout << std::endl;

		std::cout << "Present Mode Amount: " << m_presentModeAmount << std::endl;
		for (uint32_t i = 0; i < m_presentModeAmount; ++i)
			std::cout << "Present Mode #" << i << " " << m_presentModes[i] << std::endl;
		std::cout << std::endl;

		delete[] familyProps;
	}

	void checkIfInitialized()
	{
		if (!m_initialized)
			throw std::runtime_error("VulkanBase wasnt initialized");
	}
};

VulkanMain* VulkanBase::m_vulkanMain;

