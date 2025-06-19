#ifndef VULKAN_CONTEXT_H
#define VULKAN_CONTEXT_H



#include "VulkanUtils.h"
#include <optional>
// TODO:
// need to be able to add extensions easily -> look at slides for example
const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
    //VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
    VK_KHR_MULTIVIEW_EXTENSION_NAME,
    //VK_KHR_MAINTENANCE2_EXTENSION_NAME,
};

// Class responsible for managing the overall Vulkan context
class VulkanContext final
{
public:
    // Constructor: Initializes the Vulkan context with a GLFW window
    VulkanContext(GLFWwindow* window);

    // Destructor:  Consider making this virtual if you anticipate any form of inheritance (even if unintended).
    ~VulkanContext() = default;

    // Returns the Vulkan instance
    VkInstance GetInstance() const { return instance; }

    // Returns the Vulkan surface
    VkSurfaceKHR GetSurface() const { return surface; }

    // Returns the Vulkan logical device
    VkDevice GetDevice() const { return device; }

    // Returns the Vulkan physical device
    VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice.value(); } // Use optional

    // Returns the graphics queue
    VkQueue GetGraphicsQueue() const { return graphicsQueue.value(); }  // Use optional

    // Returns the presentation queue
    VkQueue GetPresentQueue() const { return presentQueue.value(); }    // Use optional

    // Returns the VMA allocator
    VmaAllocator GetVMAAllocator() const { return VMA_ALLOCATOR; }

    // Returns the command pool
    VkCommandPool GetCommandPool() const { return commandPool.value(); }      // Use optional

    // Returns the maximum buffer size supported by the device
    VkDeviceSize GetMaxBufferSize() const { return maxBufferSize; }

    // Returns the number of MSAA samples
    VkSampleCountFlagBits GetMsaaSamples() const { return msaaSamples; }

    // Cleans up all Vulkan resources managed by the context
    void CleanupContext();

private:
    // Creates the Vulkan instance
    void CreateInstance();

    // Creates the Vulkan surface for the given GLFW window
    void CreateSurface(GLFWwindow* window);

    // Sets up the Vulkan debug messenger (for validation layers)
    void SetupDebugMessenger();

    // Checks if the required validation layers are supported
    bool CheckValidationLayerSupport();

    // Populates the debug messenger create info struct
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    // Creates the Vulkan debug utils messenger extension
    VkResult CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator);

    // Gets the required Vulkan extensions based on whether validation layers are enabled
    std::vector<const char*> GetRequiredExtensions();

    // Destroys the Vulkan debug utils messenger
    void DestroyDebugUtilsMessengerEXT(const VkAllocationCallbacks* pAllocator);

    // Picks the best-suited physical device
    void PickPhysicalDevice();

    // Creates the Vulkan logical device
    void CreateLogicalDevice();

    // Checks if a physical device is suitable for the application's requirements
    bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);

    // Checks if a device supports the required extensions
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

    // Creates the VMA allocator
    void CreateVMAAllocator();

    // Creates the command pool
    void CreateCommandPool();

    // Gets the maximum number of MSAA samples supported
    VkSampleCountFlagBits GetMaxUsableSampleCount();

    // Destroys the debug messenger
    void DestroyDebugMessenger();

    // Vulkan instance handle
    VkInstance instance;

    // Vulkan surface handle
    VkSurfaceKHR surface;

    // Vulkan debug messenger handle
    VkDebugUtilsMessengerEXT debugMessenger;

    // Vulkan physical device handle
    std::optional<VkPhysicalDevice> physicalDevice = std::nullopt; 
    // Vulkan logical device handle
    VkDevice device = VK_NULL_HANDLE;

    // Graphics queue handle
    std::optional<VkQueue> graphicsQueue = std::nullopt;     
    // Presentation queue handle
    std::optional<VkQueue> presentQueue = std::nullopt;       
    // Vulkan command pool handle
    std::optional<VkCommandPool> commandPool = std::nullopt;         

    // Maximum buffer size supported by the device
    VkDeviceSize maxBufferSize;

    // MSAA sample count
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    // VMA allocator handle
    VmaAllocator VMA_ALLOCATOR;
};
#endif