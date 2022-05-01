#pragma once

#include <stdexcept>

#include "VulkanUtils.h"

class DepthImage
{
private:
	VkImage m_image = VK_NULL_HANDLE;
	VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
	VkImageView m_imageView = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;
	bool m_created = false;

public:
	DepthImage() {};

	void create(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool, VkQueue queue, VkSampleCountFlagBits sampleCount, uint32_t width, uint32_t height)
	{
		if (m_created)
			throw std::logic_error("Depth image already created");

		m_device = device;

		VkFormat depthFormat = findDepthFormat(physicalDevice);

		//
		createImage(physicalDevice, device, width, height, depthFormat, sampleCount, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_image, m_imageMemory);
		createImageView(device, m_image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, m_imageView);
		changeImageLayout(device, commandPool, queue, m_image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		
		m_created = true;
	}

	void destroy()
	{
		if (m_created)
		{
			vkDestroyImageView(m_device, m_imageView, nullptr);
			vkDestroyImage(m_device, m_image, nullptr);
			vkFreeMemory(m_device, m_imageMemory, nullptr);

			m_created = false;

			m_image = VK_NULL_HANDLE;
			m_imageMemory = VK_NULL_HANDLE;
			m_imageView = VK_NULL_HANDLE;
			m_device = VK_NULL_HANDLE;
		}
	}

	VkImageView getImageView()
	{
		return m_imageView;
	}

	static VkFormat findDepthFormat(VkPhysicalDevice physicalDevice)
	{
		std::vector<VkFormat> possibleFormats = 
		{
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D32_SFLOAT								// no stencil (old GPUs)
		};

		return findSupportedFormat(physicalDevice, possibleFormats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	static VkAttachmentDescription getDepthAttachment(VkPhysicalDevice physicalDevice, VkSampleCountFlagBits sampleCount)
	{
		VkAttachmentDescription depthAttachment;
		depthAttachment.flags = 0;
		depthAttachment.format = findDepthFormat(physicalDevice);
		depthAttachment.samples = sampleCount;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		return depthAttachment;
	}

	static VkPipelineDepthStencilStateCreateInfo getDepthStencilStateCreateInfoOpaque()
	{
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
		depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilStateCreateInfo.pNext = nullptr;
		depthStencilStateCreateInfo.flags = 0;
		depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
		depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;					// glass doesnt write in depth (transparency)
		depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
		depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
		depthStencilStateCreateInfo.front = {};
		depthStencilStateCreateInfo.back = {};
		depthStencilStateCreateInfo.minDepthBounds = 0.0f;
		depthStencilStateCreateInfo.maxDepthBounds = 1.0f;

		return depthStencilStateCreateInfo;
	}
};