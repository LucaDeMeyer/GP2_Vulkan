#include "VulkanContext.h"

#include <set>


VulkanContext::VulkanContext(GLFWwindow* window)
{
	CreateInstance();
	CreateSurface(window);
	SetupDebugMessenger();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateVMAAllocator();
	CreateCommandPool();
}

void VulkanContext::CreateSurface(GLFWwindow* window)
{
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface!");
	}
	
}

void VulkanContext::CreateInstance()
{
	if (enableValidationLayers && !CheckValidationLayerSupport())
	{
		throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Graphics Engine";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Vulkan Graphics Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = GetRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;

		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &instance))
	{
		throw std::runtime_error("failed to create instance!");
	}
}

bool VulkanContext::CheckValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : VALIDATION_LAYERS) {
		bool found = false;
		for (const auto& layer : availableLayers) {
			if (strcmp(layerName, layer.layerName) == 0) {
				found = true;
				break;
			}
		}
		if (!found) return false;
	}
	return true;
}

void VulkanContext::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = VulkanUtils::debugCallback;
}

VkResult VulkanContext::CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, &debugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}
std::vector<const char*> VulkanContext::GetRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	}

	return extensions;
}

void VulkanContext::DestroyDebugMessenger() {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func) {
		func(instance, debugMessenger, nullptr);
	}
}

void VulkanContext::DestroyDebugUtilsMessengerEXT(const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void VulkanContext::PickPhysicalDevice()
{


	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0)
		throw std::runtime_error("failed to find GPUs with Vulkan support!");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	for (const auto& device : devices) {
		if (IsDeviceSuitable(device, surface)) {
			physicalDevice = device;
			msaaSamples = GetMaxUsableSampleCount();
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
	


	VkPhysicalDeviceProperties2 deviceProperties2{};
	deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	VkPhysicalDeviceProperties& deviceProperties = deviceProperties2.properties;


	VkPhysicalDeviceMaintenance4Properties maintenance4Properties{};
	maintenance4Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES;
	deviceProperties2.pNext = &maintenance4Properties;

	vkGetPhysicalDeviceProperties2(physicalDevice.value(), &deviceProperties2);
}


void VulkanContext::CreateLogicalDevice()
{

	QueueFamilyIndices indices = VulkanUtils::FindQueueFamilies(physicalDevice.value(), surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceVulkan13Features features13{};
	features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features13.dynamicRendering = VK_TRUE;
	features13.synchronization2 = VK_TRUE;
	features13.maintenance4 = VK_TRUE;
	features13.pNext = nullptr;

	VkPhysicalDeviceVulkan12Features features12{};
	features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	features12.pNext = &features13;

	VkPhysicalDeviceVulkan11Features features11{};
	features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	features11.multiview = VK_TRUE;
	features11.pNext = &features12;

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE;

	VkPhysicalDeviceFeatures2 features2{};
	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features2.pNext = &features11;
	features2.features = deviceFeatures;

	//vkGetPhysicalDeviceFeatures2(physicalDevice.value(), &features2);

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = VK_NULL_HANDLE;
	createInfo.pNext = &features2;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physicalDevice.value(), &createInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	std::cout << "Graphics Queue Family (before get): " << indices.graphicsFamily.value() << std::endl;
	std::cout << "Present Queue Family (before get): " << indices.presentFamily.value() << std::endl;

	VkQueue tempGraphicsQueue = VK_NULL_HANDLE;
	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &tempGraphicsQueue);

	VkQueue tempPresentQueue = VK_NULL_HANDLE; 
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &tempPresentQueue);

	// Assign the retrieved handles to the optional members
	graphicsQueue = tempGraphicsQueue;
	presentQueue = tempPresentQueue;

	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue.value());
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue.value());
}

bool VulkanContext::IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	QueueFamilyIndices indices = VulkanUtils::FindQueueFamilies(device, surface);

	bool extensionSupported = CheckDeviceExtensionSupport(device);

	bool swapchainAdequate = false;

	if (extensionSupported)
	{
		SwapChainSupportDetails swapchainSupport = VulkanUtils::QuerySwapChainSupport(device, surface);

		swapchainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return indices.IsComplete() && extensionSupported && swapchainAdequate && supportedFeatures.samplerAnisotropy;



}

bool VulkanContext::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::cout << "  Checking Device Extensions:" << std::endl;
	for (const auto& ext : availableExtensions)
	{
		std::cout << "    Available: " << ext.extensionName << std::endl;
	}

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	std::cout << "  Required Extensions:" << std::endl;
	for (const auto& ext : requiredExtensions)
	{
		std::cout << "    Required: " << ext << std::endl;
	}

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	if (!requiredExtensions.empty())
	{
		std::cout << "  Missing Required Extensions:" << std::endl;
		for (const auto& ext : requiredExtensions)
		{
			std::cout << "    Missing: " << ext << std::endl;
		}
		return false;
	}

	return true;
}


void VulkanContext::CreateCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = VulkanUtils::FindQueueFamilies(physicalDevice.value(), surface);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	VkCommandPool tempCommandPool = VK_NULL_HANDLE; // Temporary variable

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &tempCommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool!");
	}

	commandPool = tempCommandPool;
}

void VulkanContext::CleanupContext()
{
	vkDestroyCommandPool(device, commandPool.value(), nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	DestroyDebugUtilsMessengerEXT(nullptr);
	vmaDestroyAllocator(VMA_ALLOCATOR);
	vkDestroyDevice(device, nullptr);
	vkDestroyInstance(instance, nullptr);
}

void VulkanContext::SetupDebugMessenger()
{
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(&createInfo, nullptr) != VK_SUCCESS)
	{
				throw std::runtime_error("failed to set up debug messenger!");
	}
}


VkSampleCountFlagBits VulkanContext::GetMaxUsableSampleCount()
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice.value(), &physicalDeviceProperties);

	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
		physicalDeviceProperties.limits.framebufferDepthSampleCounts;

	for (VkSampleCountFlagBits bit = VK_SAMPLE_COUNT_64_BIT; bit > 0; bit = (VkSampleCountFlagBits)(bit >> 1)) {
		if (counts & bit) {
			return bit;
		}
	}

	return VK_SAMPLE_COUNT_1_BIT;
}



void VulkanContext::CreateVMAAllocator()
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = physicalDevice.value();
	allocatorInfo.device = device;
	allocatorInfo.instance = instance;

	vmaCreateAllocator(&allocatorInfo, &VMA_ALLOCATOR);
}