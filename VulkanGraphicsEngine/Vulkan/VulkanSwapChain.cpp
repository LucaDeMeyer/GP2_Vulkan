#include "VulkanSwapChain.h"
#include "VulkanContext.h"
#include "Image.h"
#include <algorithm>

#include "VulkanDepthBuffer.h"
#include "WindowManager.h"



void VulkanSwapchain::CleanupSwapchain()
{
	if (context)
	{
		vkDestroyImageView(context->GetDevice(), colorImageView, nullptr);
		
		vmaDestroyImage(context->GetVMAAllocator(), colorImage, colorImageAllocation);
		
		for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
			vkDestroyFramebuffer(context->GetDevice(), swapChainFramebuffers[i], nullptr);
		}

		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			vkDestroyImageView(context->GetDevice(), swapChainImageViews[i], nullptr);
		}

		vkDestroySwapchainKHR(context->GetDevice(), swapChain, nullptr);
	}
}

VulkanSwapchain& VulkanSwapchain::CreateSwapchain()
{
	SwapChainSupportDetails swapChainSupport = VulkanUtils::QuerySwapChainSupport(context->GetPhysicalDevice(),context->GetSurface());

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = context->GetSurface();
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = VulkanUtils::FindQueueFamilies(context->GetPhysicalDevice(),context->GetSurface());
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(context->GetDevice(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(context->GetDevice(), swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(context->GetDevice(), swapChain, &imageCount, swapChainImages.data());
	


	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

	return *this;

}

VkSurfaceFormatKHR VulkanSwapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR VulkanSwapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{

			return availablePresentMode;
		}

	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{

	GLFWwindow* window = WindowManager::GetInstance().GetWindow();
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

VulkanSwapchain& VulkanSwapchain::CreateImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());
	
	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		swapChainImageViews[i] = Image::CreateImageView(context->GetDevice(),swapChainImages[i],swapChainImageFormat,VK_IMAGE_ASPECT_COLOR_BIT,1);
	}
	return *this;
}

void VulkanSwapchain::CreateFramebuffers(VkRenderPass renderPass, VkImageView depthImageView)
{
		swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++)
	{
					std::array<VkImageView, 3> attachments = {
						colorImageView,
						depthImageView,
						swapChainImageViews[i]
						
					};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(context->GetDevice(), &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void VulkanSwapchain::ReCreateSwapchain( VkRenderPass renderPass,VulkanDepthBuffer* depthBuffer)
{
	GLFWwindow* window = WindowManager::GetInstance().GetWindow();
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(context->GetDevice());


	depthBuffer->CleanupDepthBuffer();
	CleanupSwapchain();

	CreateSwapchain();
	CreateImageViews();
	CreateColorResources();
	depthBuffer->CreateDepthResources(swapChainExtent);
	//CreateFramebuffers(renderPass,depthBuffer->GetDepthImageView());
}
void VulkanSwapchain::CreateColorResources()
{
	VkFormat colorFormat = swapChainImageFormat;

	Image::CreateImage(context->GetVMAAllocator(),swapChainExtent.width, swapChainExtent.height, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, colorImage,  1, context->GetMsaaSamples(), colorImageAllocation);

	colorImageView = Image::CreateImageView(context->GetDevice(), colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}
