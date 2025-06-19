#ifndef VULKAN_DESCRIPTOR_MANAGER_H
#define VULKAN_DESCRIPTOR_MANAGER_H
#include "VulkanUtils.h"
struct Mesh;
class VulkanContext;


struct DescriptorBufferBinding
{
	uint32_t binding;
	VkBuffer buffer;
	VkDeviceSize offset = 0;
	VkDeviceSize range;

};

struct DescriptorImageBinding
{
	uint32_t binding;          
	uint32_t arrayElement = 0; 
	VkImageView imageView;
	VkSampler sampler;
	VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
};
class VulkanDescriptorManager final
{
public:

	explicit VulkanDescriptorManager(VulkanContext* context);
	~VulkanDescriptorManager() = default;

	VulkanDescriptorManager(const VulkanDescriptorManager&) = delete;
	VulkanDescriptorManager& operator=(const VulkanDescriptorManager&) = delete;
	VulkanDescriptorManager(VulkanDescriptorManager&&) = default;
	VulkanDescriptorManager& operator=(VulkanDescriptorManager&&) = default;

	// --- Pool Management ---
   // Creates the main descriptor pool. Configurable pool sizes & max sets.
	void CreateDescriptorPool(
		const std::vector<VkDescriptorPoolSize>& poolSizes,
		uint32_t maxSets,
		VkDescriptorPoolCreateFlags flags = 0 
	);

	VkDescriptorPool GetDescriptorPool() const { return descriptorPool; }

	// --- Layout Management ---
 // Creates a layout based on provided bindings. Returns the handle.
 // The caller is responsible for destroying this layout.
	VkDescriptorSetLayout CreateDescriptorSetLayout(
		const std::vector<VkDescriptorSetLayoutBinding>& bindings
	);

	// --- Set Allocation & Updating ---
	// Allocates multiple sets for a given layout and updates them using a callback.
	// The callback provides the specific buffer/image bindings for each set index.
	// Returns the allocated sets. Caller might need to free them if pool allows.
	std::vector<VkDescriptorSet> AllocateAndWriteDescriptorSets(
		VkDescriptorSetLayout layout, // The layout these sets will adhere to
		uint32_t setCount,          // How many sets to create (e.g., MAX_FRAMES_IN_FLIGHT)
		// Callback function: Takes set index (0 to setCount-1), returns bindings for THAT set
		std::function<std::pair<std::vector<DescriptorBufferBinding>, std::vector<DescriptorImageBinding>>(uint32_t setIndex)> getBindingsForSet
	);

	// --- Cleanup ---
	void CleanupPool();


private:

	VulkanContext* context;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

	};

#endif