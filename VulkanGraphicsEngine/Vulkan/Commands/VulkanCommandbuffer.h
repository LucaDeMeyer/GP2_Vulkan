#ifndef VULKAN_COMMANDBUFFER_H
#define VULKAN_COMMANDBUFFER_H

#include "VulkanContext.h"
#include <vector>


class VulkanCommandBuffer final
{
public:
	VulkanCommandBuffer(VulkanContext* context) : context(context) {}
	~VulkanCommandBuffer() = default;

	void CleanupCommandBuffers();

	const std::vector<VkCommandBuffer>& GetCommandBuffers() { return commandBuffers; }
	
	void CreateCommandBuffers();

private:

	VulkanContext* context;

	std::vector<VkCommandBuffer> commandBuffers;
};
#endif
