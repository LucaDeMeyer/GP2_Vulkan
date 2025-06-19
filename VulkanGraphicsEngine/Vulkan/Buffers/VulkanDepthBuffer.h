#ifndef VULKAN_DEPTHBUFFER_H
#define VULKAN_DEPTHBUFFER_H
#include "VulkanUtils.h"

class VulkanContext;
class VulkanDepthBuffer final
{
public:
	VulkanDepthBuffer(VulkanContext* context) : context(context) {}
	~VulkanDepthBuffer() = default;

	void CleanupDepthBuffer();

	void CreateDepthResources(VkExtent2D swapchainExtent);

	VkImageView GetDepthImageView() { return depthImageView; }
	VkImage GetDepthImage() { return depthImage; }
	VkImageView GetDepthResolveImageView() { return depthResolveImageView; }
	VkImage GetDepthResolveImage() { return DepthResolveImage; }

private:
	
	VulkanContext* context;
	VkImage depthImage = VK_NULL_HANDLE;
	VkImage DepthResolveImage = VK_NULL_HANDLE;
	VkImageView depthResolveImageView = VK_NULL_HANDLE;
	VkImageView depthImageView = VK_NULL_HANDLE;
	VmaAllocation depthImageAllocation;
	VmaAllocation depthResolveImageAllocation;
};
#endif