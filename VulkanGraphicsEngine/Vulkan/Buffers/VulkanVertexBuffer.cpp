#include "VulkanVertexBuffer.h"

#include "code/AssetLib/3MF/3MFXmlTags.h"


void VulkanVertexBuffer::CleanupVertexBuffer()
{
	if (context)
	{
		vmaDestroyBuffer(context->GetVMAAllocator(), vertexBuffer, VertexAllocation);
	}
}


void VulkanVertexBuffer::CreateVertexBuffer(std::vector<Vertex> vertices)
{

	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation{};

	VulkanUtils::CreateBuffer(context->GetVMAAllocator(),bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer, VMA_MEMORY_USAGE_CPU_ONLY, stagingBufferAllocation);

	void* data;
	vmaMapMemory(context->GetVMAAllocator(), stagingBufferAllocation, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vmaUnmapMemory(context->GetVMAAllocator(), stagingBufferAllocation);

	VulkanUtils::CreateBuffer(context->GetVMAAllocator(),bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBuffer, VMA_MEMORY_USAGE_GPU_ONLY, VertexAllocation);
	
	VulkanUtils::CopyBuffer(context->GetDevice(), stagingBuffer, vertexBuffer, bufferSize, context->GetGraphicsQueue(), context->GetCommandPool());

	vmaDestroyBuffer(context->GetVMAAllocator(), stagingBuffer, stagingBufferAllocation);
}
