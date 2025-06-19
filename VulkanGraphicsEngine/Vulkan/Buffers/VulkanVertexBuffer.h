#ifndef VULKAN_VERTEXBUFFER_H
#define VULKAN_VERTEXBUFFER_H
#include "VulkanContext.h"
#include "Scene.h"

class VulkanVertexBuffer final
{
public:

	VulkanVertexBuffer(VulkanContext* context) : context(context) {}
	~VulkanVertexBuffer() = default;

	void CreateVertexBuffer(std::vector<Vertex> vertices);
	void CleanupVertexBuffer();


	VkBuffer GetVertexBuffer() { return vertexBuffer; }
	

	//std::vector<Vertex> vertices;
	//std::vector<uint32_t> indices;

private:

	VulkanContext* context;
	VkBuffer vertexBuffer;

	VmaAllocation VertexAllocation = nullptr;
	
};

#endif