#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include <cstdint>
#include <vector>
#include "VulkanUtils.h"
#include <map>
#include "Scene.h"

class WindowManager;
class VulkanContext;
class VulkanSwapchain;
class VulkanPipeline;
class VulkanTexture;
class VulkanUniformBuffer;
class VulkanDescriptorManager;
class VulkanCommandBuffer;
class VulkanSyncObjects;
class VulkanDepthBuffer;
class ModelLoader;
class ImguiManager;
struct Mesh;
struct PipelineInfo;
class HDRManager;
class GBufferManager;

class VulkanRenderer
{
public:
	VulkanRenderer() = default;
	
	~VulkanRenderer();

	//**
	// Cleanup Vulkan resources
	//**
	void CleanupVulkan();

	//**
	// creation of vulkan object managers
	//**
	void CreateVulkanManagers();

	//**
	// Initializes all required vulkan objects using their respective managers
	//**
	void InitVulkan();

	//**
	// initializes Imgui resources
	//**
	void InitImGui();

	//**
	// Draws the frame
	//**
	void DrawFrame();

	//**
	// Main loop for the application
	//**
	void MainLoop();


	bool framebufferResized{false};

	
private:

	//**
	// Recording of commandbuffer
	//**
	void RecordCommandBuffer(uint32_t imageIndex);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	


	//**
	//Private data members
	//**
	WindowManager* window;
	ModelLoader* modelLoader;
	VulkanContext* context;
	VulkanSwapchain* swapchain;

	VulkanPipeline* pipeline;

	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	PipelineInfo* pipelineConfig;

	VulkanPipeline* hdrPipeline;
	VkPipelineLayout hdrPipelineLayout;
	VkPipeline HdrGraphicsPipeline;
	PipelineInfo* hdrPipelineConfig;

	VulkanTexture* texture;
	VulkanUniformBuffer* uniformBuffer;

	VulkanDescriptorManager* descriptorManager;
	std::vector<VkDescriptorSet> globalDescriptorSet;
	VkDescriptorSetLayout globalLayout;
	std::vector<VkDescriptorSet> hdrDescriptorSet;
	VkDescriptorSetLayout hdrDescriptorSetLayout;

	GBufferManager* gBufferManager;
	std::vector<VkDescriptorSet> gBufferDescriptorSet;
	VkDescriptorSetLayout gBufferDescriptorSetLayout;
	VulkanPipeline* gBufferPipeline;
	VkPipelineLayout gBufferPipelineLayout;
	VkPipeline gBufferGraphicsPipeline;
	PipelineInfo* gBufferPipelineConfig;
	HDRManager* hdrManager;

	
	std::vector<VkDescriptorSet> lightingDescriptorSet;
	VkDescriptorSetLayout lightingDescriptorSetLayout;
	VulkanPipeline* lightingPipeline;
	VkPipelineLayout lightingPipelineLayout;
	VkPipeline lightingGraphicsPipeline;
	PipelineInfo* lightingPipelineConfig;

	VulkanCommandBuffer* commandBuffer;
	VulkanSyncObjects* syncObjects;
	VulkanDepthBuffer* depthBuffer;
	std::vector<Mesh*> meshes;
	ImguiManager* imguiManager;

	

	Scene::Camera* camera;

	std::vector<PointLight> lights;
	DirectionalLight dirLight;

	uint32_t currentFrame{0};

	
	float currentExposure = 1.0f; 
	int currentTonemapOperator = 1; 

	bool cursorEnabled{ true };
	bool rightMouseButtonPressed = false;
	bool firstMouse = true;

};
#endif