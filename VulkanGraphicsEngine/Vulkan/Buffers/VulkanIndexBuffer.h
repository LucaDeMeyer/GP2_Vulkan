#ifndef VULKAN_INDEXBUFFER_H
#define VULKAN_INDEXBUFFER_H
#include "VulkanContext.h"

class VulkanIndexBuffer final
{
public:
	VulkanIndexBuffer(VulkanContext* context) : context(context){} 
	~VulkanIndexBuffer() = default;

	void CreateIndexBuffer(std::vector<uint32_t> indices);
	void CleanupIndexBuffer();

	VkBuffer GetIndexBuffer() { return indexBuffer; }


private:

	VulkanContext* context;


	VkBuffer indexBuffer;
	VmaAllocation indexAllocation = nullptr;
};

#endif // !VULKAN_INDEXBUFFER_H
