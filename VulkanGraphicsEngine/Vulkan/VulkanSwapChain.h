#ifndef VULKAN_SWAPCHAIN_H
#define VULKAN_SWAPCHAIN_H

#include "VulkanUtils.h"
#include <vector>
class VulkanDepthBuffer;
class VulkanContext;
class VulkanSwapchain final
{
public:
	VulkanSwapchain(VulkanContext* context) : context(context){}
	~VulkanSwapchain() = default;

	//**
	//returns swapchain
	//**
	VkSwapchainKHR GetSwapChain() const { return swapChain; }

	//**
	// Returns the format of the images in the swapchain
	//**
	VkFormat GetSwapChainImageFormat() const { return swapChainImageFormat; }

	//**
	// Returns the extent of the swapchain images
	//**
	VkExtent2D GetSwapChainExtent() const { return swapChainExtent; }

	//**
	// Returns the image views of the swapchain images
	//**
	std::vector<VkImageView> GetSwapchainImageViews() const { return swapChainImageViews; }
	//**
	// Returns the images of the swapchain
	//**
	std::vector<VkImage> GetSwapchainImages() const { return swapChainImages; }

	//**
	// Returns the color image
	//**
	VkImageView GetColorImageView() const { return colorImageView; }
	//**
	// returns frameBuffer
	//**
	std::vector<VkFramebuffer> GetSwapchainFrameBuffers() const {return swapChainFramebuffers;}

	//**
	// recreation of swapchain
	//**
	void ReCreateSwapchain(VkRenderPass renderPass, VulkanDepthBuffer* depthBuffer);

	//**
	// swapchain Creation
	//**
	VulkanSwapchain& CreateSwapchain();

	//**
	// Imgae view creation
	//**
	VulkanSwapchain& CreateImageViews();
	//**
	// clean up
	//**
	void CleanupSwapchain();
	
	//**
	//Framebuffer creation
	//**
	void CreateFramebuffers(VkRenderPass renderPass, VkImageView depthImageView);

	//**
	// Creates the resources for the offscreen color attachment
	//**
	void CreateColorResources();

	//**
	// Returns amount of images in the swapchain
	//**
	uint32_t GetSwapChainImageCount() const { return static_cast<uint32_t>(swapChainImages.size()); }
private:
	//**
	// Chooses the best surface format from the available formats
	//**
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	//**
	// Chooses the best presentation mode from the available present modes
	//**
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	//**
	// Chooses the extent (resolution) of the swapchain images based on the surface capabilities
	//**
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	

	//**
	// private data members
	//**
	VulkanContext* context;

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkImage colorImage;
	VkDeviceMemory colorImageMemory;
	VkImageView colorImageView;

	VmaAllocation colorImageAllocation{};
};

#endif