#include "VulkanIndexBuffer.h"


void VulkanIndexBuffer::CleanupIndexBuffer()
{
	if (context)
	{
		vmaDestroyBuffer(context->GetVMAAllocator(), indexBuffer, indexAllocation);
	}
}

void VulkanIndexBuffer::CreateIndexBuffer(std::vector<uint32_t> indices)
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
	VkBuffer stagingBuffer;
	
	VmaAllocation stagingAlloc{};
	VulkanUtils::CreateBuffer(context->GetVMAAllocator(),bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  stagingBuffer, VMA_MEMORY_USAGE_CPU_ONLY,stagingAlloc);

	void* data;
	vmaMapMemory(context->GetVMAAllocator(), stagingAlloc, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vmaUnmapMemory(context->GetVMAAllocator(), stagingAlloc);
	

	VulkanUtils::CreateBuffer(context->GetVMAAllocator(),bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,indexBuffer, VMA_MEMORY_USAGE_GPU_ONLY,indexAllocation);

	VulkanUtils::CopyBuffer(context->GetDevice(), stagingBuffer, indexBuffer, bufferSize, context->GetGraphicsQueue(), context->GetCommandPool());

	vmaDestroyBuffer(context->GetVMAAllocator(), stagingBuffer, stagingAlloc);
	
}


