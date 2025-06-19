#ifndef VULKAN_UNIFORMBUFFER_H
#define VULKAN_UNIFORMBUFFER_H

#include "VulkanContext.h"


struct ManagedUBO
{
	ManagedUBO() = default;
	ManagedUBO(const ManagedUBO&) = delete;
	ManagedUBO& operator=(const ManagedUBO&) = delete;


	ManagedUBO(ManagedUBO&& other) noexcept :
		buffer(other.buffer),
		allocation(other.allocation),
		mappedMemory(other.mappedMemory)
	{
		other.buffer = VK_NULL_HANDLE;
		other.allocation = VK_NULL_HANDLE;
		other.mappedMemory = nullptr;
	}

	ManagedUBO& operator=(ManagedUBO&& other) noexcept {
		if (this != &other) {
			buffer = other.buffer;
			allocation = other.allocation;
			mappedMemory = other.mappedMemory;

			other.buffer = VK_NULL_HANDLE;
			other.allocation = VK_NULL_HANDLE;
			other.mappedMemory = nullptr;
		}
		return *this;
	}

	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;
	void* mappedMemory = nullptr;


	void destroy(VmaAllocator allocator) {
		if (mappedMemory) {
			vmaUnmapMemory(allocator, allocation);
			mappedMemory = nullptr;
		}
		if (buffer != VK_NULL_HANDLE) {
			vmaDestroyBuffer(allocator, buffer, allocation);
			buffer = VK_NULL_HANDLE;
			allocation = VK_NULL_HANDLE;
		}
	}
};

class VulkanUniformBuffer final
{
public:


	VulkanUniformBuffer(VulkanContext * context) : context(context){}
	~VulkanUniformBuffer() = default;

	void CleanupUniformBuffer();


	void UpdateModelUBO(uint32_t currentFrame); 

	template<typename T>
	void UpdateUBO(uint32_t currentImage, const std::vector<ManagedUBO>& ubo, T& uboData)
	{
		memcpy(ubo[currentImage].mappedMemory, &uboData, sizeof(T));
	}
	
	void InitBuffers();

	const std::vector<ManagedUBO>& GetModelUBOs() const { return ModelUBOs; }
	const std::vector<ManagedUBO>& GetCameraUBOs() const { return CameraUBOs; }
	const std::vector<ManagedUBO>& GetSceneLightsUBOs() const { return LightUBO; }

	template<typename T>
	void CreatreUBO(std::vector<ManagedUBO>& UBO)
	{
		VkDeviceSize buffersize = sizeof(T);
		UBO.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			ManagedUBO& currentBuffer = UBO[i];

			VulkanUtils::CreateBuffer(context->GetVMAAllocator(), buffersize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				currentBuffer.buffer, VMA_MEMORY_USAGE_CPU_TO_GPU, currentBuffer.allocation);

			if (vmaMapMemory(context->GetVMAAllocator(), currentBuffer.allocation, &currentBuffer.mappedMemory) != VK_SUCCESS) {
				currentBuffer.mappedMemory = nullptr;
				throw std::runtime_error("failed to map uniform buffer memory!");
			}
		}
	}

private:
	VulkanContext* context;

	std::vector<ManagedUBO> ModelUBOs;
	std::vector<ManagedUBO> CameraUBOs;
	std::vector<ManagedUBO> LightUBO;
	
};

#endif