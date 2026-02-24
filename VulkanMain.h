#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>

#include "Pipeline.h"
#include "EasyImage.h"
#include "Mesh.h"
#include "VulkanMainConfig.h"

class VulkanMain;

void meshThreadCall(VulkanMain* vulkanMain);


class VulkanMain
{
public:
	VulkanMain() {}

	VulkanMain(VulkanMainConfig& vulkanMainConfig) : m_window(vulkanMainConfig.window), m_physicalDevice(vulkanMainConfig.physicalDevice), m_surface(vulkanMainConfig.surface),
		m_surfaceCapabilities(vulkanMainConfig.surfaceCapabilities), m_presentModes(vulkanMainConfig.presentModes),
		m_objFilePath(vulkanMainConfig.objPath), m_texFilePath(vulkanMainConfig.texFilePath)
	{
		if ((m_flags.useWireframe && m_flags.useCelShading) && !m_flags.useSplitscreen)
			throw std::runtime_error("cant use wireframe and cel shading at the same time without splitscreen true");

		m_nums.width = vulkanMainConfig.width;
		m_nums.height = vulkanMainConfig.height;
		m_flags.useSplitscreen = vulkanMainConfig.useSplitscreen;
		m_flags.useWireframe = vulkanMainConfig.useWireframe;
		m_flags.useCelShading = vulkanMainConfig.useCelShading;
	}

	~VulkanMain()
	{
		destroy();
	}

	void checkIfInitialized()
	{
		if (!m_flags.initialized)
			throw std::runtime_error("func call before init");
	}

	void init()
	{
		//std::thread t1(meshThreadCall, this);

		getMaxMSAASamples(m_msaaSampleCount, m_physicalDevice);

		createDevice();
		createSwapchain();
		createImageViews();
		createRenderPass();
		createDescriptorSetLayout();
		createPipeline();
		createCommandPool();

		///
		createColorImage();

		createDepthImage();
		createFramebuffers();
		createCommandBuffers();
		loadTexture();
		loadMesh();

		createUniformBuffer(m_ubo, m_buffers.uniformBuffer, m_deviceMemory.uniformBufferMemory);
		createUniformBuffer(m_userRes, m_buffers.resolutionUniformBuffer, m_deviceMemory.resolutionUniformBufferMemory);

		createDescriptorPool();
		createDescriptorSet();

		//t1.join();

		createVertexBuffer();
		createIndexBuffer();
		recordCommandBuffers();
		createSemaphores();

		m_flags.initialized = true;

		/*
		loadTexture();
		loadMesh();
		createVertexBuffer();
		createIndexBuffer();
		createUniformBuffer();
		createDescriptorPool();
		createDescriptorSet();
		recordCommandBuffers();
		createSemaphores();
		*/
	}

	void recreateSwapchain()
	{
		vkDeviceWaitIdle(m_device);

		m_depthImage.destroy();

		vkFreeCommandBuffers(m_device, m_commandPools.graphicsCommandPool, m_nums.amountOfImagesInSwapchain, m_commandBuffers);
		delete[] m_commandBuffers;

		for (size_t i = 0; i < m_nums.amountOfImagesInSwapchain; ++i)
			vkDestroyFramebuffer(m_device, m_framebuffers[i], nullptr);
		delete[] m_framebuffers;

		vkDestroyRenderPass(m_device, m_renderPass, nullptr);

		for (uint32_t i = 0; i < m_nums.amountOfImagesInSwapchain; ++i)
			vkDestroyImageView(m_device, m_imageViews[i], nullptr);
		delete[] m_imageViews;


		destroyColorImage();


		VkSwapchainKHR oldSwapchain = m_swapchain;

		getCapabilities();
		createSwapchain();
		createImageViews();
		createRenderPass();
		//
		createColorImage();
		createDepthImage();
		createFramebuffers();
		createCommandBuffers();
		recordCommandBuffers();

		vkDestroySwapchainKHR(m_device, oldSwapchain, nullptr);
	}

	void drawFrame()
	{
		vkQueueWaitIdle(m_queues.graphicsQueue);
		//vkQueueWaitIdle(m_queues.transferQueue);
		vkQueueWaitIdle(m_queues.presentQueue);

		uint32_t imageIndex;
		vkAcquireNextImageKHR(m_device, m_swapchain, std::numeric_limits<uint64_t>::max() - 1,
			m_semaphores.imageAvailable, VK_NULL_HANDLE, &imageIndex);				// must not std::numeric_limits<uint64_t>::max() with swapchain.minImage > 1

		VkSubmitInfo submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &m_semaphores.imageAvailable;
		VkPipelineStageFlags waitStageMask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.pWaitDstStageMask = waitStageMask;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &(m_commandBuffers[imageIndex]);
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_semaphores.renderingDone;

		VkResult result = vkQueueSubmit(m_queues.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		VK_CHECK(result);

		VkPresentInfoKHR presentInfo;
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &m_semaphores.renderingDone;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_swapchain;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		result = vkQueuePresentKHR(m_queues.presentQueue, &presentInfo);
		VK_CHECK(result);

	}

	/*glm::mat4 updateViewMatrix()
	{
		glm::mat4 rotM = glm::mat4(1.0f);
		glm::mat4 transM;

		rotM = glm::rotate(rotM, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		rotM = glm::rotate(rotM, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		rotM = glm::rotate(rotM, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		transM = glm::translate(glm::mat4(1.0f), position);

		/*if (type == CameraType::firstperson)
		{
			matrices.view = rotM * transM;
		}
		else

		return transM * rotM;
		//return rotM * transM;
	};*/

	void onMoving(int key)
	{	
		glm::vec3 camFront;
		camFront.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
		camFront.y = sin(glm::radians(rotation.x));
		camFront.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
		camFront = glm::normalize(camFront);
		
		if (key == GLFW_KEY_Q)
			position += camFront * m_nums.movementSpeed;
		if (key == GLFW_KEY_E)
			position -= camFront * m_nums.movementSpeed;
		if (key == GLFW_KEY_W)
			position += glm::normalize(glm::cross(camFront, glm::vec3(-0.5f, 0.5f, 0.0f))) * m_nums.movementSpeed;
		if (key == GLFW_KEY_S)
			position -= glm::normalize(glm::cross(camFront, glm::vec3(-0.5f, 0.5f, 0.0f))) * m_nums.movementSpeed;
		if (key == GLFW_KEY_D)
			position += glm::normalize(glm::cross(camFront, glm::vec3(1.0f, 1.0f, 0.0f))) * m_nums.movementSpeed;
		if (key == GLFW_KEY_A)
			position -= glm::normalize(glm::cross(camFront, glm::vec3(1.0f, 1.0f, 0.0f))) * m_nums.movementSpeed;
		

		m_flags.updatedViewMatrix = true;
	}

	void updateMVP()
	{
		static auto procStartTime = std::chrono::high_resolution_clock::now();
		auto frameTime = std::chrono::high_resolution_clock::now();

		float timeSinceStart = std::chrono::duration_cast<std::chrono::milliseconds>(frameTime - procStartTime).count() / 1000.0f;

		glm::vec3 offset = glm::vec3(timeSinceStart * 1.0f, 0.0f, 0.0f);

		/*			model matrix			*/
		m_ubo.model = glm::mat4(1.0f);
		m_ubo.model = glm::translate(m_ubo.model, glm::vec3(0.0f, 0.0f, -0.2f));
		m_ubo.model = glm::translate(m_ubo.model, offset);
		
		if ((m_objFilePath == "meshes//chalet.obj") || (m_objFilePath == "meshes//skull.obj"))
			m_ubo.model = glm::scale(m_ubo.model, glm::vec3(0.7f, 0.7f, 0.7f));
		else
			m_ubo.model = glm::scale(m_ubo.model, glm::vec3(0.1f, 0.1f, 0.1f));

		m_ubo.model = glm::rotate(m_ubo.model, timeSinceStart * glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		/*			view matrix				*/	
		m_ubo.view = glm::lookAt(position + offset, position - glm::vec3(1.0f, 1.0f, 1.0f) + offset, glm::vec3(0.0f, 0.0f, 1.0f));

		/*			projection matrix						*/
		if (m_flags.useSplitscreen)
			m_ubo.projection = glm::perspective(glm::radians(60.0f), (m_nums.width / 2) / static_cast<float>(m_nums.height), 0.01f, 10.0f);
		else
		{
			m_ubo.projection = glm::perspective(glm::radians(60.0f), m_nums.width / static_cast<float>(m_nums.height), 0.01f, 10.0f);
		}

		m_ubo.projection[1][1] *= -1;																							// flip y axis -> glm is for openGL, y axis flipped	

		m_ubo.lightPosition = glm::vec4(offset, 0.0f) + glm::rotate(glm::mat4(1.0f), timeSinceStart * glm::radians(60.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(0, 3, 1, 0);

		void* data;
		vkMapMemory(m_device, m_deviceMemory.uniformBufferMemory, 0, sizeof(m_ubo), 0, &data);
		memcpy(data, &m_ubo, sizeof(m_ubo));
		vkUnmapMemory(m_device, m_deviceMemory.uniformBufferMemory);

		m_userRes.x = m_nums.width;
		m_userRes.y = m_nums.height;

		if (m_flags.useSplitscreen)
			m_userRes.x /= 2;

		vkMapMemory(m_device, m_deviceMemory.resolutionUniformBufferMemory, 0, sizeof(m_userRes), 0, &data);
		memcpy(data, &m_userRes, sizeof(m_userRes));
		vkUnmapMemory(m_device, m_deviceMemory.resolutionUniformBufferMemory);
	}

	void destroy()
	{
		vkDeviceWaitIdle(m_device);

		m_depthImage.destroy();

		m_inputImage.destroy();

		destroyColorImage();

		vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

		/////
		vkFreeMemory(m_device, m_deviceMemory.resolutionUniformBufferMemory, nullptr);
		vkDestroyBuffer(m_device, m_buffers.resolutionUniformBuffer, nullptr);

		vkFreeMemory(m_device, m_deviceMemory.uniformBufferMemory, nullptr);
		vkDestroyBuffer(m_device, m_buffers.uniformBuffer, nullptr);
		vkFreeMemory(m_device, m_deviceMemory.indexBufferDeviceMemory, nullptr);
		vkDestroyBuffer(m_device, m_buffers.indexBuffer, nullptr);
		vkFreeMemory(m_device, m_deviceMemory.vertexBufferDeviceMemory, nullptr);
		vkDestroyBuffer(m_device, m_buffers.vertexBuffer, nullptr);

		vkDestroySemaphore(m_device, m_semaphores.renderingDone, nullptr);
		vkDestroySemaphore(m_device, m_semaphores.imageAvailable, nullptr);
		vkFreeCommandBuffers(m_device, m_commandPools.graphicsCommandPool, m_nums.amountOfImagesInSwapchain, m_commandBuffers);
		delete[] m_commandBuffers;

		vkDestroyCommandPool(m_device, m_commandPools.transferCommandPool, nullptr);
		vkDestroyCommandPool(m_device, m_commandPools.graphicsCommandPool, nullptr);

		for (uint32_t i = 0; i < m_nums.amountOfImagesInSwapchain; ++i)
			vkDestroyFramebuffer(m_device, m_framebuffers[i], nullptr);
		delete[] m_framebuffers;

		m_pipelines.leftPipeline.destroy();
		m_pipelines.rightPipeline.destroy();
		m_pipelines.wholePipeline.destroy();

		vkDestroyRenderPass(m_device, m_renderPass, nullptr);
		vkDestroyShaderModule(m_device, m_shaderModules.vert, nullptr);
		vkDestroyShaderModule(m_device, m_shaderModules.frag, nullptr);
		vkDestroyShaderModule(m_device, m_shaderModules.fragWireframe, nullptr);
		vkDestroyShaderModule(m_device, m_shaderModules.fragCelShading, nullptr);
		for (uint32_t i = 0; i < m_nums.amountOfImagesInSwapchain; ++i)
			vkDestroyImageView(m_device, m_imageViews[i], nullptr);
		delete[] m_imageViews;
		vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

		vkDestroyDevice(m_device, nullptr);
	}

	void setWidth(uint32_t width)
	{
		m_nums.width = width;
	}

	void setHeight(uint32_t height)
	{
		m_nums.height = height;
	}

	void getCapabilities()
	{
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &m_surfaceCapabilities);

		uint32_t presentModeAmount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeAmount, nullptr);

		m_presentModes = new VkPresentModeKHR[presentModeAmount];
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeAmount, m_presentModes);
	}

	void loadMesh()
	{
		m_mesh.create(m_objFilePath);
		m_vertices = m_mesh.getVertices();
		m_indices = m_mesh.getIndices();
	}

	void loadTexture()
	{
		m_inputImage.load(m_texFilePath);
		m_inputImage.upload(m_device, m_physicalDevice, m_commandPools.transferCommandPool, m_queues.transferQueue, VK_SAMPLE_COUNT_1_BIT);
	}

	VkDevice getDevice()
	{
		checkIfInitialized();
		return m_device;
	}

private:
	const char* m_objFilePath;
	const char* m_texFilePath;

	GLFWwindow* m_window;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;
	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
	VkImageView* m_imageViews = nullptr;
	VkFramebuffer* m_framebuffers = nullptr;
	VkRenderPass m_renderPass = VK_NULL_HANDLE;
	VkCommandBuffer* m_commandBuffers = nullptr;

	VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

	EasyImage m_inputImage;
	DepthImage m_depthImage;

	const VkFormat m_rgbFormat = VK_FORMAT_B8G8R8A8_UNORM;

	std::vector<Vertex> m_vertices = {};
	std::vector<uint32_t> m_indices = {};
	glm::vec3 position = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 rotation = glm::vec3(1.0f, 1.0f, 1.0f);

	Mesh m_mesh;

	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	VkSurfaceCapabilitiesKHR m_surfaceCapabilities = {};
	VkPresentModeKHR* m_presentModes;

	VkSampleCountFlagBits m_msaaSampleCount;
	VkImage m_colorImage;
	VkImageView m_colorImageView;

	struct nums
	{
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t amountOfImagesInSwapchain = 0;
		float movementSpeed = 1.0f;
		float rotationSpeed = 1.0f;

	} m_nums;

	struct flags
	{
		bool initialized = false;
		bool useWireframe = false;
		bool useCelShading = false;
		bool useSplitscreen = false;
		bool updatedViewMatrix = false;

	} m_flags;

	struct queues
	{
		VkQueue graphicsQueue = VK_NULL_HANDLE;
		VkQueue transferQueue = VK_NULL_HANDLE;
		VkQueue presentQueue = VK_NULL_HANDLE;

	} m_queues;

	struct commandPools
	{
		VkCommandPool graphicsCommandPool = VK_NULL_HANDLE;
		VkCommandPool transferCommandPool = VK_NULL_HANDLE;

	} m_commandPools;

	struct buffers
	{
		VkBuffer vertexBuffer = VK_NULL_HANDLE;
		VkBuffer indexBuffer = VK_NULL_HANDLE;
		VkBuffer uniformBuffer = VK_NULL_HANDLE;
		VkBuffer resolutionUniformBuffer = VK_NULL_HANDLE;

	} m_buffers;

	struct deviceMemory
	{
		VkDeviceMemory vertexBufferDeviceMemory = VK_NULL_HANDLE;
		VkDeviceMemory indexBufferDeviceMemory = VK_NULL_HANDLE;
		VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;
		VkDeviceMemory resolutionUniformBufferMemory = VK_NULL_HANDLE;
		VkDeviceMemory colorImageMemory = VK_NULL_HANDLE;

	} m_deviceMemory;

	struct pipelines
	{
		Pipeline leftPipeline;
		Pipeline rightPipeline;
		Pipeline wholePipeline;

	} m_pipelines;

	struct shaderModules
	{
		VkShaderModule vert = VK_NULL_HANDLE;
		VkShaderModule frag = VK_NULL_HANDLE;
		VkShaderModule fragWireframe = VK_NULL_HANDLE;
		VkShaderModule fragCelShading = VK_NULL_HANDLE;

	} m_shaderModules;

	struct semaphores
	{
		VkSemaphore imageAvailable = VK_NULL_HANDLE;
		VkSemaphore renderingDone = VK_NULL_HANDLE;

	} m_semaphores;

	struct UniformBufferObject
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;
		glm::vec3 lightPosition;

	} m_ubo;

	struct userResolution
	{
		int x = 0;
		int y = 0;

	} m_userRes;



	std::vector<char> readFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::binary | std::ios::ate);

		if (file)
		{
			size_t fileSize = (size_t)file.tellg();
			std::vector<char> fileBuffer(fileSize);
			file.seekg(0);
			file.read(fileBuffer.data(), fileSize);
			file.close();

			return fileBuffer;
		}
		else
			throw std::runtime_error("Cant open file");
	}

	void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule)
	{
		VkShaderModuleCreateInfo shaderCreateInfo;
		shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderCreateInfo.pNext = nullptr;
		shaderCreateInfo.flags = 0;
		shaderCreateInfo.codeSize = code.size();
		shaderCreateInfo.pCode = (uint32_t*)code.data();

		VkResult result = vkCreateShaderModule(m_device, &shaderCreateInfo, nullptr, shaderModule);
		VK_CHECK(result);
	}

	void createDevice()
	{
		float queuePrios[] = { 0.0f, 1.0f };

		VkDeviceQueueCreateInfo deviceGraphicsQueueCreateInfo;
		deviceGraphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceGraphicsQueueCreateInfo.pNext = nullptr;
		deviceGraphicsQueueCreateInfo.flags = 0;
		deviceGraphicsQueueCreateInfo.queueFamilyIndex = 0;				// TODO: choose dynamically
		deviceGraphicsQueueCreateInfo.queueCount = 2;					// TODO: choose dynamically
		deviceGraphicsQueueCreateInfo.pQueuePriorities = queuePrios;

		VkDeviceQueueCreateInfo deviceTransferQueueCreateInfo;
		deviceTransferQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceTransferQueueCreateInfo.pNext = nullptr;
		deviceTransferQueueCreateInfo.flags = 0;
		deviceTransferQueueCreateInfo.queueFamilyIndex = 1;				// TODO: choose dynamically
		deviceTransferQueueCreateInfo.queueCount = 1;					// TODO: choose dynamically
		deviceTransferQueueCreateInfo.pQueuePriorities = queuePrios;


		std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
		deviceQueueCreateInfos.push_back(deviceGraphicsQueueCreateInfo);
		deviceQueueCreateInfos.push_back(deviceTransferQueueCreateInfo);


		/////////////////////////
		VkPhysicalDeviceFeatures availableFeatures;
		vkGetPhysicalDeviceFeatures(m_physicalDevice, &availableFeatures);

		VkPhysicalDeviceFeatures usedFeatures = {};

		if (availableFeatures.samplerAnisotropy == VK_TRUE)
			usedFeatures.samplerAnisotropy = VK_TRUE;
		else
			throw std::runtime_error("feature fail");				// TODO

		if (availableFeatures.fillModeNonSolid == VK_TRUE)
			usedFeatures.fillModeNonSolid = VK_TRUE;
		else
			throw std::runtime_error("feature fail");				// TODO

		if (availableFeatures.sampleRateShading == VK_TRUE)
			usedFeatures.sampleRateShading = VK_TRUE;
		else
			throw std::runtime_error("feature fail");

		const std::vector<const char*> deviceExtensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		VkDeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pNext = nullptr;
		deviceCreateInfo.flags = 0;
		deviceCreateInfo.queueCreateInfoCount = deviceQueueCreateInfos.size();											// TODO: dynamically
		deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();						// TODO: dynamically
		deviceCreateInfo.enabledLayerCount = 0;												// TODO
		deviceCreateInfo.ppEnabledLayerNames = nullptr;										// TODO
		deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();					// TODO
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();					// TODO
		deviceCreateInfo.pEnabledFeatures = &usedFeatures;

		VkResult result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);
		VK_CHECK(result);

		vkGetDeviceQueue(m_device, 0, 0, &m_queues.graphicsQueue);
		vkGetDeviceQueue(m_device, 0, 1, &m_queues.presentQueue);
		vkGetDeviceQueue(m_device, 1, 0, &m_queues.transferQueue);

		VkBool32 surfaceSupport = false;
		result = vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, 0, m_surface, &surfaceSupport);
		VK_CHECK(result);

		if (!surfaceSupport)
		{
			std::cerr << "Surface not supported" << std::endl;
			__debugbreak();
		}
	}

	void createSwapchain()
	{
		VkSwapchainCreateInfoKHR swapchainCreateInfo;
		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.pNext = nullptr;
		swapchainCreateInfo.flags = 0;
		swapchainCreateInfo.surface = m_surface;
		swapchainCreateInfo.minImageCount = m_surfaceCapabilities.minImageCount;
		swapchainCreateInfo.imageFormat = m_rgbFormat;
		swapchainCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;								// TODO
		swapchainCreateInfo.imageExtent = m_surfaceCapabilities.currentExtent;
		swapchainCreateInfo.imageArrayLayers = m_surfaceCapabilities.maxImageArrayLayers;
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
		swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCreateInfo.presentMode = m_presentModes[2];												// TODO
		swapchainCreateInfo.clipped = VK_TRUE;
		swapchainCreateInfo.oldSwapchain = m_swapchain;

		VkResult result = vkCreateSwapchainKHR(m_device, &swapchainCreateInfo, nullptr, &m_swapchain);
		VK_CHECK(result);

	}

	void createImageViews()
	{
		vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_nums.amountOfImagesInSwapchain, nullptr);
		VkImage* swapchainImages = new VkImage[m_nums.amountOfImagesInSwapchain];
		VkResult result = vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_nums.amountOfImagesInSwapchain, swapchainImages);
		VK_CHECK(result);

		m_imageViews = new VkImageView[m_nums.amountOfImagesInSwapchain];
		for (uint32_t i = 0; i < m_nums.amountOfImagesInSwapchain; ++i)
			createImageView(m_device, swapchainImages[i], m_rgbFormat, VK_IMAGE_ASPECT_COLOR_BIT, m_imageViews[i]);

		delete[] swapchainImages;
	}

	void createRenderPass()
	{
		// renderpass (attachment = memory area)
		VkAttachmentDescription attachmentDescription;
		attachmentDescription.flags = 0;
		attachmentDescription.format = m_rgbFormat;
		attachmentDescription.samples = m_msaaSampleCount;
		attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment = DepthImage::getDepthAttachment(m_physicalDevice, m_msaaSampleCount);

		VkAttachmentDescription colorAttachmentResolve;
		colorAttachmentResolve.flags = 0;
		colorAttachmentResolve.format = m_rgbFormat;
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference attachmentReference;
		attachmentReference.attachment = 0;
		attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentReference;
		depthAttachmentReference.attachment = 1;
		depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentResolveRef;
		colorAttachmentResolveRef.attachment = 2;
		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// subpass = 'single draw call'
		VkSubpassDescription subpassDescription;
		subpassDescription.flags = 0;
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.inputAttachmentCount = 0;
		subpassDescription.pInputAttachments = nullptr;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &attachmentReference;
		subpassDescription.pResolveAttachments = &colorAttachmentResolveRef;
		subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
		subpassDescription.preserveAttachmentCount = 0;
		subpassDescription.pPreserveAttachments = nullptr;

		VkSubpassDependency subpassDependency;
		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass = 0;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.srcAccessMask = 0;
		subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependency.dependencyFlags = 0;

		std::vector<VkAttachmentDescription> attachments;
		attachments.push_back(attachmentDescription);
		attachments.push_back(depthAttachment);
		attachments.push_back(colorAttachmentResolve);

		VkRenderPassCreateInfo renderpassCreateInfo;
		renderpassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderpassCreateInfo.pNext = nullptr;
		renderpassCreateInfo.flags = 0;
		renderpassCreateInfo.attachmentCount = attachments.size();
		renderpassCreateInfo.pAttachments = attachments.data();
		renderpassCreateInfo.subpassCount = 1;														// TODO
		renderpassCreateInfo.pSubpasses = &subpassDescription;
		renderpassCreateInfo.dependencyCount = 1;
		renderpassCreateInfo.pDependencies = &subpassDependency;

		VkResult result = vkCreateRenderPass(m_device, &renderpassCreateInfo, nullptr, &m_renderPass);
		VK_CHECK(result);

	}

	void createDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding descriptorSetLayoutBinding;
		descriptorSetLayoutBinding.binding = 0;														// binding index (shader.vert)
		descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorSetLayoutBinding.descriptorCount = 1;
		descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding samplerDescriptorSetLayoutBinding;
		samplerDescriptorSetLayoutBinding.binding = 1;
		samplerDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerDescriptorSetLayoutBinding.descriptorCount = 1;
		samplerDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		samplerDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding descriptorSetLayoutResolutionBinding;
		descriptorSetLayoutResolutionBinding.binding = 2;
		descriptorSetLayoutResolutionBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorSetLayoutResolutionBinding.descriptorCount = 1;
		descriptorSetLayoutResolutionBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		descriptorSetLayoutResolutionBinding.pImmutableSamplers = nullptr;

		std::vector<VkDescriptorSetLayoutBinding> descriptorSets;
		descriptorSets.push_back(descriptorSetLayoutBinding);
		descriptorSets.push_back(samplerDescriptorSetLayoutBinding);
		descriptorSets.push_back(descriptorSetLayoutResolutionBinding);

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.pNext = nullptr;
		descriptorSetLayoutCreateInfo.flags = 0;
		descriptorSetLayoutCreateInfo.bindingCount = descriptorSets.size();
		descriptorSetLayoutCreateInfo.pBindings = descriptorSets.data();

		VkResult result = vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCreateInfo, nullptr, &m_descriptorSetLayout);
		VK_CHECK(result);
	}

	void createPipeline()
	{
		auto shaderCodeVert = readFile("vertShader.spv");
		auto shaderCodeFrag = readFile("fragShader.spv");
		auto shaderCodeFragWireframe = readFile("wireframeFragShader.spv");
		auto shaderCodeFragCelShading = readFile("cel_shading.spv");

		createShaderModule(shaderCodeVert, &m_shaderModules.vert);
		createShaderModule(shaderCodeFrag, &m_shaderModules.frag);
		createShaderModule(shaderCodeFragWireframe, &m_shaderModules.fragWireframe);
		createShaderModule(shaderCodeFragCelShading, &m_shaderModules.fragCelShading);

		VkFrontFace frontFace;
		if (m_objFilePath == "meshes//chalet.obj")
			frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		else
			frontFace = VK_FRONT_FACE_CLOCKWISE;

		if (m_flags.useSplitscreen)
		{
			if (m_flags.useWireframe && m_flags.useCelShading)
			{
				m_pipelines.leftPipeline.init(frontFace, m_shaderModules.vert, m_shaderModules.fragWireframe, m_msaaSampleCount, m_nums.width, m_nums.height);
				m_pipelines.leftPipeline.setPolygonMode(VK_POLYGON_MODE_LINE);
				m_pipelines.leftPipeline.create(m_device, m_renderPass, m_descriptorSetLayout);

				m_pipelines.rightPipeline.init(frontFace, m_shaderModules.vert, m_shaderModules.fragCelShading, m_msaaSampleCount, m_nums.width, m_nums.height);
				m_pipelines.rightPipeline.create(m_device, m_renderPass, m_descriptorSetLayout);
			}
			else
			{
				if (m_flags.useWireframe)
				{
					m_pipelines.leftPipeline.init(frontFace, m_shaderModules.vert, m_shaderModules.frag, m_msaaSampleCount, m_nums.width, m_nums.height);
					m_pipelines.leftPipeline.create(m_device, m_renderPass, m_descriptorSetLayout);

					m_pipelines.rightPipeline.init(frontFace, m_shaderModules.vert, m_shaderModules.fragWireframe, m_msaaSampleCount, m_nums.width, m_nums.height);
					m_pipelines.rightPipeline.setPolygonMode(VK_POLYGON_MODE_LINE);
					m_pipelines.rightPipeline.create(m_device, m_renderPass, m_descriptorSetLayout);
				}
				else if (m_flags.useCelShading)
				{
					m_pipelines.leftPipeline.init(frontFace, m_shaderModules.vert, m_shaderModules.frag, m_msaaSampleCount, m_nums.width, m_nums.height);
					m_pipelines.leftPipeline.create(m_device, m_renderPass, m_descriptorSetLayout);

					m_pipelines.rightPipeline.init(frontFace, m_shaderModules.vert, m_shaderModules.fragCelShading, m_msaaSampleCount, m_nums.width, m_nums.height);
					m_pipelines.rightPipeline.create(m_device, m_renderPass, m_descriptorSetLayout);
				}
				else
				{
					m_pipelines.leftPipeline.init(frontFace, m_shaderModules.vert, m_shaderModules.frag, m_msaaSampleCount, m_nums.width, m_nums.height);
					m_pipelines.leftPipeline.create(m_device, m_renderPass, m_descriptorSetLayout);

					m_pipelines.rightPipeline.init(frontFace, m_shaderModules.vert, m_shaderModules.frag, m_msaaSampleCount, m_nums.width, m_nums.height);
					m_pipelines.rightPipeline.create(m_device, m_renderPass, m_descriptorSetLayout);
				}
			}

		}
		else
		{
			if (m_flags.useWireframe)
			{
				m_pipelines.wholePipeline.init(frontFace, m_shaderModules.vert, m_shaderModules.fragWireframe, m_msaaSampleCount, m_nums.width, m_nums.height);
				m_pipelines.wholePipeline.setPolygonMode(VK_POLYGON_MODE_LINE);
				m_pipelines.wholePipeline.create(m_device, m_renderPass, m_descriptorSetLayout);
			}
			else if (m_flags.useCelShading)
			{
				m_pipelines.wholePipeline.init(frontFace, m_shaderModules.vert, m_shaderModules.fragCelShading, m_msaaSampleCount, m_nums.width, m_nums.height);
				m_pipelines.wholePipeline.create(m_device, m_renderPass, m_descriptorSetLayout);
			}
			else
			{
				m_pipelines.wholePipeline.init(frontFace, m_shaderModules.vert, m_shaderModules.frag, m_msaaSampleCount, m_nums.width, m_nums.height);
				m_pipelines.wholePipeline.create(m_device, m_renderPass, m_descriptorSetLayout);
			}
		}
	}

	void createCommandPool()
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo;
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.pNext = nullptr;
		commandPoolCreateInfo.flags = 0;
		commandPoolCreateInfo.queueFamilyIndex = 0;									// TODO dynamically choose queueFamily with GRAPHICS_BIT

		VkCommandPoolCreateInfo transferCommandPoolCreateInfo;
		transferCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		transferCommandPoolCreateInfo.pNext = nullptr;
		transferCommandPoolCreateInfo.flags = 0;
		transferCommandPoolCreateInfo.queueFamilyIndex = 1;

		VkResult result = vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &m_commandPools.graphicsCommandPool);
		VK_CHECK(result);
		result = vkCreateCommandPool(m_device, &transferCommandPoolCreateInfo, nullptr, &m_commandPools.transferCommandPool);
		VK_CHECK(result);
	}

	void createColorImage()
	{
		createColorResources(m_physicalDevice, m_device, m_colorImage, m_deviceMemory.colorImageMemory, m_colorImageView, m_rgbFormat, m_msaaSampleCount, m_nums.width, m_nums.height);
	}

	void createDepthImage()
	{
		m_depthImage.create(m_physicalDevice, m_device, m_commandPools.transferCommandPool, m_queues.transferQueue, m_msaaSampleCount, m_nums.width, m_nums.height);
	}

	void createFramebuffers()
	{
		m_framebuffers = new VkFramebuffer[m_nums.amountOfImagesInSwapchain];

		VkResult result;
		for (size_t i = 0; i < m_nums.amountOfImagesInSwapchain; ++i)
		{
			std::vector<VkImageView> attachmentViews;
			attachmentViews.push_back(m_colorImageView);
			attachmentViews.push_back(m_depthImage.getImageView());
			attachmentViews.push_back(m_imageViews[i]);

			VkFramebufferCreateInfo framebufferCreateInfo;
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.pNext = nullptr;
			framebufferCreateInfo.flags = 0;
			framebufferCreateInfo.renderPass = m_renderPass;
			framebufferCreateInfo.attachmentCount = attachmentViews.size();
			framebufferCreateInfo.pAttachments = attachmentViews.data();
			framebufferCreateInfo.width = m_nums.width;
			framebufferCreateInfo.height = m_nums.height;
			framebufferCreateInfo.layers = 1;

			result = vkCreateFramebuffer(m_device, &framebufferCreateInfo, nullptr, &m_framebuffers[i]);
			VK_CHECK(result);
		}
	}

	void createCommandBuffers()
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo;
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.pNext = nullptr;
		commandBufferAllocateInfo.commandPool = m_commandPools.graphicsCommandPool;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount = m_nums.amountOfImagesInSwapchain;

		m_commandBuffers = new VkCommandBuffer[m_nums.amountOfImagesInSwapchain];
		VkResult result = vkAllocateCommandBuffers(m_device, &commandBufferAllocateInfo, m_commandBuffers);
		VK_CHECK(result);
	}

	void createVertexBuffer()
	{
		vkQueueWaitIdle(m_queues.transferQueue);
		createAndUploadBuffer(m_physicalDevice, m_device, m_queues.transferQueue, m_commandPools.transferCommandPool, m_vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			m_buffers.vertexBuffer, m_deviceMemory.vertexBufferDeviceMemory);
	}

	void createIndexBuffer()
	{
		vkQueueWaitIdle(m_queues.transferQueue);
		createAndUploadBuffer(m_physicalDevice, m_device, m_queues.transferQueue, m_commandPools.transferCommandPool, m_indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			m_buffers.indexBuffer, m_deviceMemory.indexBufferDeviceMemory);
	}

	template <typename T>
	void createUniformBuffer(T bufferObject, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
	{
		VkDeviceSize bufferSize = sizeof(bufferObject);
		createBuffer(m_physicalDevice, m_device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, bufferMemory);
	}

	void createDescriptorPool()
	{
		VkDescriptorPoolSize descriptorPoolSize;
		descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorPoolSize.descriptorCount = 1;

		VkDescriptorPoolSize samplerPoolSize;
		samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerPoolSize.descriptorCount = 1;

		VkDescriptorPoolSize userResolutionPoolSize;
		userResolutionPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		userResolutionPoolSize.descriptorCount = 1;

		std::vector<VkDescriptorPoolSize> descriptorPoolSizes;
		descriptorPoolSizes.push_back(descriptorPoolSize);
		descriptorPoolSizes.push_back(samplerPoolSize);
		descriptorPoolSizes.push_back(userResolutionPoolSize);

		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
		descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCreateInfo.pNext = nullptr;
		descriptorPoolCreateInfo.flags = 0;
		descriptorPoolCreateInfo.maxSets = 1;
		descriptorPoolCreateInfo.poolSizeCount = descriptorPoolSizes.size();
		descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();

		VkResult result = vkCreateDescriptorPool(m_device, &descriptorPoolCreateInfo, nullptr, &m_descriptorPool);
		VK_CHECK(result);
	}

	void createDescriptorSet()
	{
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.pNext = nullptr;
		descriptorSetAllocateInfo.descriptorPool = m_descriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.pSetLayouts = &m_descriptorSetLayout;

		VkResult result = vkAllocateDescriptorSets(m_device, &descriptorSetAllocateInfo, &m_descriptorSet);
		VK_CHECK(result);

		VkDescriptorBufferInfo descriptorBufferInfo;
		descriptorBufferInfo.buffer = m_buffers.uniformBuffer;
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range = sizeof(m_ubo);

		VkWriteDescriptorSet descriptorWrite;
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.pNext = nullptr;
		descriptorWrite.dstSet = m_descriptorSet;
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.pImageInfo = nullptr;
		descriptorWrite.pBufferInfo = &descriptorBufferInfo;
		descriptorWrite.pTexelBufferView = nullptr;

		VkDescriptorImageInfo descriptorImageInfo;
		descriptorImageInfo.sampler = m_inputImage.getSampler();
		descriptorImageInfo.imageView = m_inputImage.getImageView();
		descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet descriptorSamplerWrite;
		descriptorSamplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorSamplerWrite.pNext = nullptr;
		descriptorSamplerWrite.dstSet = m_descriptorSet;
		descriptorSamplerWrite.dstBinding = 1;
		descriptorSamplerWrite.dstArrayElement = 0;
		descriptorSamplerWrite.descriptorCount = 1;
		descriptorSamplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorSamplerWrite.pImageInfo = &descriptorImageInfo;
		descriptorSamplerWrite.pBufferInfo = nullptr;
		descriptorSamplerWrite.pTexelBufferView = nullptr;

		VkDescriptorBufferInfo descriptorBufferResolutionInfo;
		descriptorBufferResolutionInfo.buffer = m_buffers.resolutionUniformBuffer;
		descriptorBufferResolutionInfo.offset = 0;
		descriptorBufferResolutionInfo.range = sizeof(m_userRes);

		VkWriteDescriptorSet descriptorResolutionWrite;
		descriptorResolutionWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorResolutionWrite.pNext = nullptr;
		descriptorResolutionWrite.dstSet = m_descriptorSet;
		descriptorResolutionWrite.dstBinding = 2;
		descriptorResolutionWrite.dstArrayElement = 0;
		descriptorResolutionWrite.descriptorCount = 1;
		descriptorResolutionWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorResolutionWrite.pImageInfo = nullptr;
		descriptorResolutionWrite.pBufferInfo = &descriptorBufferResolutionInfo;
		descriptorResolutionWrite.pTexelBufferView = nullptr;

		std::vector<VkWriteDescriptorSet> writeDescriptorSets;
		writeDescriptorSets.push_back(descriptorWrite);
		writeDescriptorSets.push_back(descriptorSamplerWrite);
		writeDescriptorSets.push_back(descriptorResolutionWrite);

		vkUpdateDescriptorSets(m_device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
	}

	void recordCommandBuffers()
	{
		VkCommandBufferBeginInfo commandBufferBeginInfo;
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.pNext = nullptr;
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		commandBufferBeginInfo.pInheritanceInfo = nullptr;								// matters when commandBuffer is secondary

		VkRenderPassBeginInfo renderPassBeginInfo;
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = m_renderPass;
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = { m_nums.width, m_nums.height };

		VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };
		VkClearValue depthClearValue = { 1.0f, 0.0f };
		std::vector<VkClearValue> clearValues;
		clearValues.push_back(clearValue);
		clearValues.push_back(depthClearValue);

		renderPassBeginInfo.clearValueCount = clearValues.size();
		renderPassBeginInfo.pClearValues = clearValues.data();

		for (size_t i = 0; i < m_nums.amountOfImagesInSwapchain; ++i)
		{
			VkResult result = vkBeginCommandBuffer(m_commandBuffers[i], &commandBufferBeginInfo);
			VK_CHECK(result);

			renderPassBeginInfo.framebuffer = m_framebuffers[i];
			vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkBool32 usePhong = VK_TRUE;

			if ((m_objFilePath != "meshes//chalet.obj") && (m_objFilePath != "meshes//skull.obj") && (m_objFilePath != "meshes//SnowTerrain.obj"))
			{
				VkBool32 noTextures = VK_TRUE;

				if (m_flags.useSplitscreen)
					vkCmdPushConstants(m_commandBuffers[i], m_pipelines.leftPipeline.getLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(VkBool32), sizeof(noTextures), &noTextures);
				else
					vkCmdPushConstants(m_commandBuffers[i], m_pipelines.wholePipeline.getLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(VkBool32), sizeof(noTextures), &noTextures);
			}

			if (m_flags.useSplitscreen)
			{
				vkCmdPushConstants(m_commandBuffers[i], m_pipelines.leftPipeline.getLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(usePhong), &usePhong);
				vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines.leftPipeline.getPipeline());
			}
			else
			{
				vkCmdPushConstants(m_commandBuffers[i], m_pipelines.wholePipeline.getLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(usePhong), &usePhong);
				vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines.wholePipeline.getPipeline());
			}

			VkViewport viewport;
			viewport.x = 0.0f;
			viewport.y = 0.0f;

			if (m_flags.useSplitscreen)
				viewport.width = float(m_nums.width / 2);
			else
				viewport.width = float(m_nums.width);

			viewport.height = float(m_nums.height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			vkCmdSetViewport(m_commandBuffers[i], 0, 1, &viewport);

			VkRect2D scissor;
			scissor.offset = { 0, 0 };
			scissor.extent = { m_nums.width, m_nums.height };
			vkCmdSetScissor(m_commandBuffers[i], 0, 1, &scissor);

			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, &m_buffers.vertexBuffer, offsets);
			vkCmdBindIndexBuffer(m_commandBuffers[i], m_buffers.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			if (m_flags.useSplitscreen)
				vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines.leftPipeline.getLayout(), 0, 1, &m_descriptorSet, 0, nullptr);
			else
				vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines.wholePipeline.getLayout(), 0, 1, &m_descriptorSet, 0, nullptr);

			vkCmdDrawIndexed(m_commandBuffers[i], m_indices.size(), 1, 0, 0, 0);

			if (m_flags.useSplitscreen)
			{
				vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines.rightPipeline.getPipeline());

				viewport.x = m_nums.width / 2.0f;
				vkCmdSetViewport(m_commandBuffers[i], 0, 1, &viewport);
				//vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, &m_buffers.vertexBuffer, offsets);			// necessary??
				//vkCmdBindIndexBuffer(m_commandBuffers[i], m_buffers.indexBuffer, 0, VK_INDEX_TYPE_UINT32);		// necessary??
				vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines.rightPipeline.getLayout(), 0, 1, &m_descriptorSet, 0, nullptr);
				vkCmdDrawIndexed(m_commandBuffers[i], m_indices.size(), 1, 0, 0, 0);
			}

			vkCmdEndRenderPass(m_commandBuffers[i]);

			result = vkEndCommandBuffer(m_commandBuffers[i]);
			VK_CHECK(result);
		}
	}

	void createSemaphores()
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo;
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCreateInfo.pNext = nullptr;
		semaphoreCreateInfo.flags = 0;

		VkResult result = vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_semaphores.imageAvailable);
		VK_CHECK(result);
		result = vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_semaphores.renderingDone);
		VK_CHECK(result);
	}

	void destroyColorImage()
	{
		vkFreeMemory(m_device, m_deviceMemory.colorImageMemory, nullptr);
		vkDestroyImage(m_device, m_colorImage, nullptr);
		vkDestroyImageView(m_device, m_colorImageView, nullptr);
	}
};


void meshThreadCall(VulkanMain* vulkanMain)
{
	vulkanMain->loadMesh();
}

