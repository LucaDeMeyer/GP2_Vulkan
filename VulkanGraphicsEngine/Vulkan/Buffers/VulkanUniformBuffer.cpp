#include "VulkanUniformBuffer.h"

#include <chrono>
#include "Scene.h"

void VulkanUniformBuffer::CleanupUniformBuffer()
{
	if (!context || !context->GetVMAAllocator()) {
		// Handle case where context is invalid or allocator not present (e.g., during shutdown)
		return;
	}

	VmaAllocator allocator = context->GetVMAAllocator();

	// Clean up Model UBOs
	for (auto& managedBuffer : ModelUBOs) {
		managedBuffer.destroy(allocator);
	}
	ModelUBOs.clear(); // Clear the vector after destroying contents

	// Clean up Camera UBOs
	for (auto& managedBuffer : CameraUBOs) {
		managedBuffer.destroy(allocator);
	}
	CameraUBOs.clear();

	// Clean up Scene Lights UBOs
	for (auto& managedBuffer : LightUBO) {
		managedBuffer.destroy(allocator);
	}
	LightUBO.clear();
	

}

void VulkanUniformBuffer::InitBuffers()
{
	CreatreUBO<ModelUBO>(ModelUBOs);
	CreatreUBO<CameraUBO>(CameraUBOs);
	CreatreUBO<SceneLightingUBO>(LightUBO);
}



void VulkanUniformBuffer::UpdateModelUBO(uint32_t currentImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	glm::mat4 model = glm::mat4(1.0f);
	ModelUBO ubo{};
	//ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::mat4{ 1.0f };
	//model = glm::translate(model, glm::vec3(0.0f, 0.0f, -5.0f)); // Move 5 units forward along the camera's view direction (-Z world axis)
	//model = glm::scale(model, glm::vec3(0.01f));

	ubo.model = model;
	memcpy(ModelUBOs[currentImage].mappedMemory, &ubo, sizeof(ubo));
}




