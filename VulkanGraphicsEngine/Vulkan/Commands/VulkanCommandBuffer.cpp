#include "VulkanCommandBuffer.h"
#include <stdexcept>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "VulkanRenderer.h"

void VulkanCommandBuffer::CleanupCommandBuffers()
{
	if (context)
	{
		vkFreeCommandBuffers(context->GetDevice(), context->GetCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
	}
}

void VulkanCommandBuffer::CreateCommandBuffers()
{
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = context->GetCommandPool();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(context->GetDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}



