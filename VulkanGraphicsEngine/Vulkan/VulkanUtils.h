#ifndef VULKAN_UTILS_H
#define VULKAN_UTILS_H

#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN  
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <vulkan/vulkan.h>
#include <vector>
#include <iostream>
#include <optional>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <array>


#include<vma/vk_mem_alloc.h>

const std::vector<const char*> VALIDATION_LAYERS = {
"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const int MAX_FRAMES_IN_FLIGHT = 2;


struct SwapChainSupportDetails { 
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices { 
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool IsComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
};



struct ModelUBO
{
	alignas(16) glm::mat4 model;
};

struct CameraUBO
{
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::vec3 cameraPos;
	alignas(4) float exposure;
};

struct hash_pair {
	template <typename T1, typename T2>
	size_t operator()(const std::pair<T1, T2>& p) const {
		return std::hash<T1>()(p.first) ^ (std::hash<T2>()(p.second) << 1);
	}
};
struct PointLight
{
	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 color;
	alignas(4) float lumen;
	alignas(4) float radius;
};
struct DirectionalLight
{
	alignas(16) glm::vec3 direction;
	alignas(16) glm::vec3 color;
	alignas(4) float lux;
};

struct SceneLightingUBO
{
	PointLight lights[4];
	DirectionalLight directionalLight; 
	alignas(4) int numberOfLights;
};

struct PushConstantData
{

	alignas(16)glm::mat4 transform;
	alignas(16)glm::mat4 modelMatrix;
};

struct ToneMapPush {
	alignas(4)float exposure;
	alignas(4)int tonemapOperator;
};

struct ScreenSizePush {
	alignas(16)glm::vec2 inverseScreenSize;
	alignas(16)glm::mat4 inverseViewProjection;
};

enum class TextureType {
	ALBEDO,
	NORMAL,
	METALLIC,
	ROUGHNESS,
	AO,
	DEFAULT,
	COUNT // Keep track of total types
};

class VulkanUtils final
{
public:
	VulkanUtils() = delete;

    static SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
	static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

    static std::vector<char> ReadFile(const std::string& filename);

	static uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

	static void CreateBuffer(VmaAllocator vmaAllocator,VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer& buffer, VmaMemoryUsage memUsage, VmaAllocation& vmaAllocation);
	static void CopyBuffer(VkDevice device, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkQueue graphicsQueue, VkCommandPool commandPool);


	static VkCommandBuffer BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
	static void EndSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandBuffer commandBuffer, VkCommandPool commandPool);

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	static VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	static VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice);

	static bool HasStencilComponent(VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	static bool IsDepthFormat(VkFormat format);
};


#endif