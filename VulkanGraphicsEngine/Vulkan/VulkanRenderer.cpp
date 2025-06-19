#include "VulkanRenderer.h"

#include <syncstream>

#include "VulkanSwapChain.h"
#include "VulkanPipeline.h"
#include "VulkanCommandBuffer.h"
#include "VulkanTexture.h"
#include "VulkanUniformBuffer.h"
#include "VulkanDescriptorManager.h"
#include "VulkanSyncObjects.h"
#include "WindowManager.h"
#include "VulkanContext.h"
#include "VulkanDepthBuffer.h"
#include "Scene.h"
#include "Image.h"
#include "HDRManager.h"
#include "GBufferManager.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imconfig.h>
#include <imgui_internal.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

static float FPS = 0;




VulkanRenderer::~VulkanRenderer()
{
	
	delete swapchain;
	delete pipeline;
	delete commandBuffer;

	for (Mesh* mesh : meshes)
	{
		delete mesh;
	}

	//delete texture;
	delete uniformBuffer;
	delete syncObjects;
	delete descriptorManager;
	delete depthBuffer;
	delete context;
}

void VulkanRenderer::CreateVulkanManagers()
{

	window = &WindowManager::GetInstance();
	window->InitWindow(WIDTH, HEIGHT, "Vulkan");
	window->SetRenderer(this);

	glfwSetWindowUserPointer(window->GetWindow(), this);

	camera = new Scene::Camera(glm::vec3(0.f, 0.f, 2.f), 45.f);

	modelLoader = &ModelLoader::GetInstance();
	context = new VulkanContext(window->GetWindow());
	swapchain = new VulkanSwapchain(context);
	pipeline = new VulkanPipeline(context);
	hdrPipeline = new VulkanPipeline(context);
	texture = new VulkanTexture(context);

	meshes.push_back(new Mesh(context));
	
	uniformBuffer = new VulkanUniformBuffer(context);
	depthBuffer = new VulkanDepthBuffer(context);
	descriptorManager = new VulkanDescriptorManager(context);
	commandBuffer = new VulkanCommandBuffer(context);
	syncObjects = new VulkanSyncObjects(context);
	hdrManager = new HDRManager(context, swapchain, pipeline, descriptorManager);
	pipelineConfig = new PipelineInfo();
	hdrPipelineConfig = new PipelineInfo();
	hdrPipeline = new VulkanPipeline(context);
	lightingPipeline = new VulkanPipeline(context);
	lightingPipelineConfig = new PipelineInfo();

	gBufferManager = new GBufferManager(context);
	gBufferPipeline = new VulkanPipeline(context);
}

void VulkanRenderer::InitVulkan()
{
	

	swapchain->CreateSwapchain()
				.CreateImageViews();

	hdrManager->Initialize();

	gBufferManager->Initialize(swapchain->GetSwapChainExtent(),1);
	gBufferManager->CreateGBufferResources(swapchain->GetSwapChainExtent());

	float aspectRatio = (float)swapchain->GetSwapChainExtent().width / swapchain->GetSwapChainExtent().height;
	camera->InitCamera(aspectRatio, 45.0f, { 0.0f, 0.0f, 3.0f });

	lights.push_back({ { 20.0f, 10.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } , 100,10 });
	lights.push_back({ { 10.0f, 5.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } ,100,20});
	lights.push_back({ { -10.0f, 5.0f, 0.0f }, { 0.0f, 0.0f, 1.0f },100,3 });
	lights.push_back({ { 0.0f, 5.0f, 10.0f }, { 1.0f, 1.0f, 1.0f },100,8000 });

	dirLight.direction = { -0.5f, -1, -0.3};
	dirLight.color = { 1.0f, 1.0f, 1.0f };
	dirLight.lux = 50000.f; 

	std::vector<VkDescriptorPoolSize> poolSize = {
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT * 3}, // 3 -> amount of uniform buffers
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAMES_IN_FLIGHT * (7 + 1 + 5)}, // 7 -> amount of samplers + 2 for imgui
	};

	uint32_t maxTotalSets = MAX_FRAMES_IN_FLIGHT * 3; 

	descriptorManager->CreateDescriptorPool(poolSize, maxTotalSets,VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

	std::vector<VkDescriptorSetLayoutBinding> globalBinding{
		{0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
		{1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT,nullptr},
		{2,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
		{3,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
		{4,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
		{5,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
		{6,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
		{7,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr},
		{8,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}
	};
	 globalLayout = descriptorManager->CreateDescriptorSetLayout(globalBinding);

	 std::vector<VkDescriptorSetLayoutBinding> HDRDescriptorSetBinding{

		{0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr}
	 };

	 hdrDescriptorSetLayout = descriptorManager->CreateDescriptorSetLayout(HDRDescriptorSetBinding);

	 std::vector<VkDescriptorSetLayoutBinding> lightingDescriptorSetBinding{
		{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}, 
		{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
		{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}, 
		{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}, 
		{4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}, 
		{8, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}, 
		{9, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}  
	 };
	 lightingDescriptorSetLayout = descriptorManager->CreateDescriptorSetLayout(lightingDescriptorSetBinding);


	 std::vector<VkFormat> gBufferFormats = {
	  gBufferManager->GetAlbedoImageFormat(),       
	  gBufferManager->GetAOImageFormat(),           
	  gBufferManager->GetNormalImageFormat(),       
	  gBufferManager->GetMetallicRoughnessImageFormat(), 
	  VK_FORMAT_R32G32B32A32_SFLOAT
	 };

	
	 pipelineConfig->colorAttachmentFormats = gBufferFormats;
	 pipeline->DefaultPipelineConfig(*pipelineConfig,pipelineConfig->colorAttachmentFormats); 


	 pipelineConfig->depthStencilInfo.depthTestEnable = VK_TRUE; 
	 pipelineConfig->depthStencilInfo.depthWriteEnable = VK_TRUE;
	 pipelineConfig->depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; 
	 pipelineConfig->depthAttachmentFormat = VulkanUtils::FindDepthFormat(context->GetPhysicalDevice()); 
	 pipelineConfig->stencilAttachmentFormat = VK_FORMAT_UNDEFINED; 

	 pipelineConfig->colorBlendAttachments.resize(gBufferFormats.size());
	 for (auto& attachment : pipelineConfig->colorBlendAttachments) {
		 attachment.blendEnable = VK_FALSE;
		 attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			 VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	 }

	 pipelineConfig->colorBlendInfo.attachmentCount = static_cast<uint32_t>(pipelineConfig->colorBlendAttachments.size());
	 pipelineConfig->colorBlendInfo.pAttachments = pipelineConfig->colorBlendAttachments.data();
	 pipeline->CreatePipelineCache()
		 .CreateGraphicsPipeline<PushConstantData>("Shaders/shader.vert.spv", "Shaders/shader.frag.spv", *pipelineConfig, globalLayout, graphicsPipeline, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);



		std::vector<VkFormat> lightingformat = { VK_FORMAT_R32G32B32A32_SFLOAT };
		lightingPipeline->DefaultPipelineConfig(*lightingPipelineConfig, lightingPipelineConfig->colorAttachmentFormats);
		lightingPipelineConfig->inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		lightingPipelineConfig->rasterizationInfo.cullMode = VK_CULL_MODE_NONE; 
		lightingPipelineConfig->depthStencilInfo.depthTestEnable = VK_FALSE;
		lightingPipelineConfig->depthStencilInfo.depthWriteEnable = VK_FALSE; 
		lightingPipelineConfig->multisampleInfo.rasterizationSamples = context->GetMsaaSamples(); 
		lightingPipelineConfig->bindingDescriptions = {}; 
		lightingPipelineConfig->attributeDescriptions = {}; 
		lightingPipelineConfig->colorAttachmentFormats = lightingformat; 
		lightingPipelineConfig->depthAttachmentFormat = VK_FORMAT_UNDEFINED; 
		lightingPipelineConfig->stencilAttachmentFormat = VK_FORMAT_UNDEFINED; 
		lightingPipeline->CreatePipelineCache()
			.CreateGraphicsPipeline<ScreenSizePush>("Shaders/fullscreen_quad.vert.spv", "Shaders/lighting.frag.spv", *lightingPipelineConfig, lightingDescriptorSetLayout, lightingGraphicsPipeline, lightingPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT);

		std::vector<VkFormat> hdrColorFormats = { swapchain->GetSwapChainImageFormat() };
		hdrPipeline->DefaultPipelineConfig(*hdrPipelineConfig, hdrPipelineConfig->colorAttachmentFormats);
		hdrPipelineConfig->multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; 
		hdrPipelineConfig->bindingDescriptions = {};
		hdrPipelineConfig->attributeDescriptions = {};
		hdrPipelineConfig->colorAttachmentFormats = hdrColorFormats;
		hdrPipelineConfig->depthAttachmentFormat = VK_FORMAT_UNDEFINED;
		hdrPipelineConfig->stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
		hdrPipeline->CreatePipelineCache()
			.CreateGraphicsPipeline<ToneMapPush>("Shaders/tonemap.vert.spv", "Shaders/tonemap.frag.spv", *hdrPipelineConfig, hdrDescriptorSetLayout, HdrGraphicsPipeline, hdrPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT);


	swapchain->CreateColorResources();
	depthBuffer->CreateDepthResources(swapchain->GetSwapChainExtent());

	//modelLoader->LoadModel("Models/gltf/sponza/Sponza.gltf", meshes, context);
	modelLoader->LoadModel("Models/gltf/flightHelmet/FlightHelmet.gltf", meshes,context);

	for (Mesh* mesh : meshes)
	{
		mesh->CreateBuffers();
	}

	uniformBuffer->InitBuffers();

	std::vector<ModelUBO> uboData;
	uboData.resize(MAX_FRAMES_IN_FLIGHT);

	std::vector<CameraUBO> cameraUboData;
	cameraUboData.resize(MAX_FRAMES_IN_FLIGHT);

	std::vector<SceneLightingUBO> lightUboData;
	lightUboData.resize(MAX_FRAMES_IN_FLIGHT);

		globalDescriptorSet = descriptorManager->AllocateAndWriteDescriptorSets(globalLayout, MAX_FRAMES_IN_FLIGHT,
		[&](uint32_t setIndex) {
			std::vector<DescriptorBufferBinding> bufferBindings = {
				{0, uniformBuffer->GetCameraUBOs()[setIndex].buffer, 0, sizeof(CameraUBO)},
				{1, uniformBuffer->GetModelUBOs()[setIndex].buffer, 0, sizeof(ModelUBO)},
				{8, uniformBuffer->GetSceneLightsUBOs()[setIndex].buffer, 0, sizeof(SceneLightingUBO)}
			};
			std::vector<DescriptorImageBinding> imageBindings = {
			
				{2, 0, meshes[0]->GetTexture(TextureType::ALBEDO).GetTextureImageView(),
				meshes[0]->GetTexture(TextureType::ALBEDO).GetTextureSampler(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
				{3, 0, meshes[0]->GetTexture(TextureType::NORMAL).GetTextureImageView(),
				meshes[0]->GetTexture(TextureType::NORMAL).GetTextureSampler(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
				{4, 0, meshes[0]->GetTexture(TextureType::METALLIC).GetTextureImageView(),
				 meshes[0]->GetTexture(TextureType::METALLIC).GetTextureSampler(),
				 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
				{5, 0, meshes[0]->GetTexture(TextureType::ROUGHNESS).GetTextureImageView(),
				 meshes[0]->GetTexture(TextureType::ROUGHNESS).GetTextureSampler(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
				 {6, 0, meshes[0]->GetTexture(TextureType::AO).GetTextureImageView(),
					meshes[0]->GetTexture(TextureType::AO).GetTextureSampler(),
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
			};
			return std::make_pair(bufferBindings, imageBindings);
		});

		hdrDescriptorSet = descriptorManager->AllocateAndWriteDescriptorSets(hdrDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT,
		[&](uint32_t setIndex) {
			std::vector<DescriptorBufferBinding> bufferBindings = {};
			std::vector<DescriptorImageBinding> imageBindings = {
				{0, 0, hdrManager->GetHDRResolveView(),
				hdrManager->GetHDRSampler(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
			};
			return std::make_pair(bufferBindings, imageBindings);
		});

	lightingDescriptorSet = descriptorManager->AllocateAndWriteDescriptorSets(lightingDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT,
		[&](uint32_t setIndex) {
			std::vector<DescriptorBufferBinding> bufferBindings = {
				{8, uniformBuffer->GetSceneLightsUBOs()[setIndex].buffer, 0, sizeof(SceneLightingUBO)},
				{9, uniformBuffer->GetCameraUBOs()[setIndex].buffer, 0, sizeof(CameraUBO)}
			};
			std::vector<DescriptorImageBinding> imageBindings = {
				{0, 0, gBufferManager->GetAlbedoImageResolveView(), gBufferManager->GetGBufferSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
				{1, 0, gBufferManager->GetAOImageResolveView(), gBufferManager->GetGBufferSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
				{2, 0, gBufferManager->GetNormalImageResolveView(), gBufferManager->GetGBufferSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
				{3, 0, gBufferManager->GetMetallicRoughnessImageResolveView(), gBufferManager->GetGBufferSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
				{4, 0, depthBuffer->GetDepthResolveImageView(), gBufferManager->GetGBufferSampler(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL}
			};
			return std::make_pair(bufferBindings, imageBindings);
		});


	commandBuffer->CreateCommandBuffers();

	syncObjects->CreateSyncObjects();


	glfwSetInputMode(window->GetWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	glfwSetMouseButtonCallback(window->GetWindow(), mouseButtonCallback);
}

void VulkanRenderer::InitImGui( )
{
	//ImGui::CreateContext();
	//
	//ImGui_ImplGlfw_InitForVulkan(WindowManager::GetInstance().GetWindow(), true);
	//ImGui_ImplVulkan_InitInfo init_info = {};
	//init_info.Instance = context->GetInstance();
	//init_info.PhysicalDevice = context->GetPhysicalDevice();
	//init_info.Device = context->GetDevice();
	//init_info.ImageCount = 2;
	//init_info.MinImageCount = 2;
	//init_info.MSAASamples = context->GetMsaaSamples();
	//init_info.Queue = context->GetGraphicsQueue();
	//init_info.DescriptorPool = descriptorManager->GetDescriptorPool();
	//init_info.RenderPass = VK_NULL_HANDLE/*pipeline->GetRenderPass()*/;
	//init_info.UseDynamicRendering = true;
	//
	//
	//VkPipelineRenderingCreateInfoKHR renderingInfo{};
	//renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	//
	//renderingInfo.pColorAttachmentFormats = *pipelineConfig->colorAttachmentFormats.data();
	//renderingInfo.colorAttachmentCount = 1;
	//renderingInfo.depthAttachmentFormat = pipelineConfig->depthAttachmentFormat;
	//renderingInfo.stencilAttachmentFormat = pipelineConfig->stencilAttachmentFormat;
	//renderingInfo.pNext = nullptr;
	//init_info.PipelineRenderingCreateInfo = renderingInfo;
	//
	//
	//ImGui_ImplVulkan_Init(&init_info);
	//
	//VkCommandBuffer commandBuffer = VulkanUtils::BeginSingleTimeCommands(context->GetDevice(), context->GetCommandPool());
	//
	//ImGui_ImplVulkan_CreateFontsTexture();
	//VulkanUtils::EndSingleTimeCommands(context->GetDevice(), context->GetGraphicsQueue(), commandBuffer, context->GetCommandPool());
	//
	//vkDeviceWaitIdle(context->GetDevice());
	//
	//ImGui_ImplVulkan_DestroyFontsTexture();
}


void VulkanRenderer::CleanupVulkan()
{

	//ImGui_ImplVulkan_Shutdown();
	//ImGui_ImplGlfw_Shutdown();
	//ImGui::DestroyContext();

	swapchain->CleanupSwapchain();
	depthBuffer->CleanupDepthBuffer();
	uniformBuffer->CleanupUniformBuffer();

	
	// -- clean up descriptor sets -- //
	vkDestroyDescriptorSetLayout(context->GetDevice(), globalLayout, nullptr);
	vkDestroyDescriptorSetLayout(context->GetDevice(), hdrDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(context->GetDevice(), lightingDescriptorSetLayout, nullptr); 
	descriptorManager->CleanupPool();


	for (Mesh * mesh : meshes)
	{
		mesh->CleanUpMesh();
	}
	pipeline->CleanupPipeline(graphicsPipeline, pipelineLayout); 
	lightingPipeline->CleanupPipeline(lightingGraphicsPipeline, lightingPipelineLayout);
	hdrPipeline->CleanupPipeline(HdrGraphicsPipeline, hdrPipelineLayout); 
	hdrManager->Cleanup();
	gBufferManager->CleanupGBuffer(); 
	syncObjects->CleanupSyncObjects();
	commandBuffer->CleanupCommandBuffers();

	context->CleanupContext();

	glfwDestroyWindow(window->GetWindow());

	glfwTerminate();
}

void VulkanRenderer::MainLoop()
{

	double lastTime = glfwGetTime();
	int frameCount = 0;

	while (!glfwWindowShouldClose(window->GetWindow()))
	{
		glfwPollEvents();

		double currentTime = glfwGetTime();
		float deltaTime = static_cast<float>(currentTime - lastTime);
		frameCount++;

		if (currentTime - lastTime >= 1.0) {
			FPS = frameCount / static_cast<float>((currentTime - lastTime));
			frameCount = 0;
			lastTime = currentTime;
		}

		// Get current mouse position
		double xpos, ypos;
		glfwGetCursorPos(window->GetWindow(), &xpos, &ypos);

		// Process camera movement based on mouse input and right button press
		camera->ProcessMouseMovement(window->GetWindow(), xpos, ypos, rightMouseButtonPressed);
		camera->ProcessKeyboard(window->GetWindow(), deltaTime);

		// Recalculate view and projection matrices
		camera->CalcViewMatrix();
		camera->aspectRatio = (float)swapchain->GetSwapChainExtent().width / swapchain->GetSwapChainExtent().height;
		camera->calculateProjectionMatrix();

		DrawFrame();
	}

	vkDeviceWaitIdle(context->GetDevice());
	
}


void VulkanRenderer::DrawFrame()
{
	VkFence inFlightFence = syncObjects->GetInFlightFence(currentFrame);
	vkWaitForFences(context->GetDevice(), 1, &inFlightFence, VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(context->GetDevice(), swapchain->GetSwapChain(), UINT64_MAX, syncObjects->GetImageAvailableSemaphore(currentFrame), VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		swapchain->ReCreateSwapchain(VK_NULL_HANDLE, depthBuffer);
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	uniformBuffer->UpdateModelUBO(currentFrame);


	// Update Camera UBO with the camera's matrices
	CameraUBO cameraUbo{};
	cameraUbo.view = camera->getView();
	cameraUbo.proj = camera->getProjection();

	SceneLightingUBO sceneLightingUbo{};
	sceneLightingUbo.lights[0] = lights[0];
	sceneLightingUbo.lights[1] = lights[1];
	sceneLightingUbo.lights[2] = lights[2];
	sceneLightingUbo.lights[3] = lights[3];
	sceneLightingUbo.numberOfLights = 4;

	sceneLightingUbo.directionalLight = dirLight;


	uniformBuffer->UpdateUBO<CameraUBO>(currentFrame,uniformBuffer->GetCameraUBOs(), cameraUbo);
	uniformBuffer->UpdateUBO<SceneLightingUBO>(currentFrame, uniformBuffer->GetSceneLightsUBOs(), sceneLightingUbo);

	RecordCommandBuffer(imageIndex);


	vkResetFences(context->GetDevice(), 1, &inFlightFence);

	VkSemaphore waitSemaphores[] = { syncObjects->GetImageAvailableSemaphore(currentFrame) };
	VkSemaphore signalSemaphores[] = { syncObjects->GetRenderFinishedSemaphore(currentFrame) };

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer->GetCommandBuffers()[currentFrame];
	
	
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	
	if (vkQueueSubmit(context->GetGraphicsQueue(), 1, &submitInfo, syncObjects->GetInFlightFence(currentFrame)) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapchain->GetSwapChain() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(context->GetPresentQueue(), &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		;
		swapchain->ReCreateSwapchain(VK_NULL_HANDLE,depthBuffer);
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}



void VulkanRenderer::RecordCommandBuffer(uint32_t imageIndex)
{
	VkCommandBuffer commandBufferCurrentFrame = commandBuffer->GetCommandBuffers()[currentFrame];
	VkExtent2D SwapchainExtent = swapchain->GetSwapChainExtent();

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if (vkBeginCommandBuffer(commandBufferCurrentFrame, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	// --- memory barrier --- //
	Image::RecordImageTransition(commandBufferCurrentFrame,gBufferManager->GetAlbedoImage(), gBufferManager->GetAlbedoImageFormat(), gBufferManager->GetAlbedoImageLayout(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
	Image::RecordImageTransition(commandBufferCurrentFrame,gBufferManager->GetAOImage(), gBufferManager->GetAOImageFormat(), gBufferManager->GetAOImageLayout(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
	Image::RecordImageTransition(commandBufferCurrentFrame,gBufferManager->GetNormalImage(), gBufferManager->GetNormalImageFormat(), gBufferManager->GetNormalImageLayout(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
	Image::RecordImageTransition(commandBufferCurrentFrame,gBufferManager->GetMetallicRoughnessImage(), gBufferManager->GetMetallicRoughnessImageFormat(), gBufferManager->GetMetallicRoughnessImageLayout(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);

	Image::RecordImageTransition(commandBufferCurrentFrame,gBufferManager->GetGWorldPosImage(), VK_FORMAT_R32G32B32A32_SFLOAT, gBufferManager->GetGWorldPosImageLayout(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
	Image::RecordImageTransition(commandBufferCurrentFrame,gBufferManager->GetAlbedoImageResolve(), gBufferManager->GetAlbedoImageFormat(),gBufferManager->GetAlbedoImageResolveLayout(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
	Image::RecordImageTransition(commandBufferCurrentFrame,gBufferManager->GetAOImageResolve(), gBufferManager->GetAOImageFormat(),gBufferManager->GetAOImageResolveLayout(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
	Image::RecordImageTransition(commandBufferCurrentFrame,gBufferManager->GetNormalImageResolve(), gBufferManager->GetNormalImageFormat(),gBufferManager->GetNormalImageResolveLayout(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
	Image::RecordImageTransition(commandBufferCurrentFrame,gBufferManager->GetMetallicRoughnessImageResolve(), gBufferManager->GetMetallicRoughnessImageFormat(),gBufferManager->GetMetallicRoughnessImageResolveLayout(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
	Image::RecordImageTransition(commandBufferCurrentFrame,depthBuffer->GetDepthResolveImage(), VulkanUtils::FindDepthFormat(context->GetPhysicalDevice()),VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
	Image::RecordImageTransition(commandBufferCurrentFrame,gBufferManager->GetGWorldPosResolveImage(), VK_FORMAT_R32G32B32A32_SFLOAT, gBufferManager->GetGWorldPosResolveImageLayout(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);

	gBufferManager->SetAlbedoImageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	gBufferManager->SetAOImageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	gBufferManager->SetNormalImageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	gBufferManager->SetMetallicRoughnessImageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	gBufferManager->SetGWorldPosImageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	gBufferManager->SetAlbedoImageResolveLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	gBufferManager->SetAOImageResolveLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	gBufferManager->SetNormalImageResolveLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	gBufferManager->SetMetallicRoughnessImageResolveLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	std::vector<VkRenderingAttachmentInfo> gBufferColorAttachments;
	gBufferColorAttachments.resize(5); 
	gBufferColorAttachments[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	gBufferColorAttachments[0].imageView = gBufferManager->GetAlbedoImageView();
	gBufferColorAttachments[0].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	gBufferColorAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	gBufferColorAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	gBufferColorAttachments[0].resolveImageView = gBufferManager->GetAlbedoImageResolveView();
	gBufferColorAttachments[0].resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	gBufferColorAttachments[0].resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT; 
	gBufferColorAttachments[0].clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

	gBufferColorAttachments[1].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	gBufferColorAttachments[1].imageView = gBufferManager->GetAOImageView();
	gBufferColorAttachments[1].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	gBufferColorAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	gBufferColorAttachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	gBufferColorAttachments[1].resolveImageView = gBufferManager->GetAOImageResolveView();
	gBufferColorAttachments[1].resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	gBufferColorAttachments[1].resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
	gBufferColorAttachments[1].clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

	gBufferColorAttachments[2].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	gBufferColorAttachments[2].imageView = gBufferManager->GetNormalImageView();
	gBufferColorAttachments[2].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	gBufferColorAttachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	gBufferColorAttachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	gBufferColorAttachments[2].resolveImageView = gBufferManager->GetNormalImageResolveView();
	gBufferColorAttachments[2].resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	gBufferColorAttachments[2].resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
	gBufferColorAttachments[2].clearValue = { 0.5f, 0.5f, 1.0f};

	gBufferColorAttachments[3].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	gBufferColorAttachments[3].imageView = gBufferManager->GetMetallicRoughnessImageView();
	gBufferColorAttachments[3].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	gBufferColorAttachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	gBufferColorAttachments[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	gBufferColorAttachments[3].resolveImageView = gBufferManager->GetMetallicRoughnessImageResolveView();
	gBufferColorAttachments[3].resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	gBufferColorAttachments[3].resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
	gBufferColorAttachments[3].clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

	gBufferColorAttachments[4].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	gBufferColorAttachments[4].imageView = gBufferManager->GetGWorldPosImageView();
	gBufferColorAttachments[4].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	gBufferColorAttachments[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	gBufferColorAttachments[4].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	gBufferColorAttachments[4].resolveImageView = gBufferManager->GetGWorldPosResolveImageView();
	gBufferColorAttachments[4].resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	gBufferColorAttachments[4].resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
	gBufferColorAttachments[4].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f }; 

	VkRenderingAttachmentInfo depthAttachmentInfo{};
	depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depthAttachmentInfo.imageView = depthBuffer->GetDepthImageView();
	depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE; 
	depthAttachmentInfo.resolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
	depthAttachmentInfo.resolveImageView = depthBuffer->GetDepthResolveImageView();
	depthAttachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; 
	depthAttachmentInfo.clearValue.depthStencil = { 1.0f, 0 };

	VkRenderingInfo gBufferRenderingInfo{};
	gBufferRenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	gBufferRenderingInfo.renderArea = VkRect2D{ VkOffset2D {0, 0}, SwapchainExtent.width, SwapchainExtent.height };
	gBufferRenderingInfo.layerCount = 1;
	gBufferRenderingInfo.colorAttachmentCount = static_cast<uint32_t>(gBufferColorAttachments.size());
	gBufferRenderingInfo.pColorAttachments = gBufferColorAttachments.data();
	gBufferRenderingInfo.pDepthAttachment = &depthAttachmentInfo;
	gBufferRenderingInfo.pStencilAttachment = VK_NULL_HANDLE; 

	vkCmdBeginRendering(commandBufferCurrentFrame, &gBufferRenderingInfo);

	vkCmdBindPipeline(commandBufferCurrentFrame, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline); 


	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)SwapchainExtent.width;
	viewport.height = (float)SwapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBufferCurrentFrame, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = SwapchainExtent;
	vkCmdSetScissor(commandBufferCurrentFrame, 0, 1, &scissor);

	VkDeviceSize offsets[] = { 0 };

	for (Mesh* mesh : meshes)
	{
		PushConstantData push{};
		push.modelMatrix = glm::mat4(1.0f); 

		vkCmdPushConstants(commandBufferCurrentFrame, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantData), &push);

		mesh->Bind(commandBufferCurrentFrame, *offsets);
		vkCmdBindDescriptorSets(commandBufferCurrentFrame, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &globalDescriptorSet[currentFrame], 0, nullptr);
		vkCmdDrawIndexed(commandBufferCurrentFrame, static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0, 0);
	}
	vkCmdEndRendering(commandBufferCurrentFrame);

	
	Image::RecordImageTransition(commandBufferCurrentFrame,gBufferManager->GetAlbedoImageResolve(), gBufferManager->GetAlbedoImageFormat(), gBufferManager->GetAlbedoImageResolveLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
	Image::RecordImageTransition(commandBufferCurrentFrame,gBufferManager->GetAOImageResolve(), gBufferManager->GetAOImageFormat(), gBufferManager->GetAOImageResolveLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
	Image::RecordImageTransition(commandBufferCurrentFrame,gBufferManager->GetNormalImageResolve(), gBufferManager->GetNormalImageFormat(), gBufferManager->GetNormalImageResolveLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
	Image::RecordImageTransition(commandBufferCurrentFrame,gBufferManager->GetMetallicRoughnessImageResolve(), gBufferManager->GetMetallicRoughnessImageFormat(), gBufferManager->GetMetallicRoughnessImageResolveLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
	Image::RecordImageTransition(commandBufferCurrentFrame,depthBuffer->GetDepthResolveImage(), VulkanUtils::FindDepthFormat(context->GetPhysicalDevice()), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, 1);
	Image::RecordImageTransition(commandBufferCurrentFrame,hdrManager->GetHDRResolve(), VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
	
	gBufferManager->SetAlbedoImageResolveLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	gBufferManager->SetAOImageResolveLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	gBufferManager->SetNormalImageResolveLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	gBufferManager->SetMetallicRoughnessImageResolveLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// --- Pass 2: Lighting Pass (Render to HDR Image) ---
	VkRenderingAttachmentInfo lightingColorAttachmentInfo{};
	lightingColorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	lightingColorAttachmentInfo.imageView = hdrManager->GetHDRMsaaView(); 
	lightingColorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	lightingColorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	lightingColorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	lightingColorAttachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	lightingColorAttachmentInfo.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
	lightingColorAttachmentInfo.resolveImageView = hdrManager->GetHDRResolveView(); 
	lightingColorAttachmentInfo.clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

	VkRenderingInfo lightingRenderingInfo{};
	lightingRenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	lightingRenderingInfo.renderArea = VkRect2D{ VkOffset2D {0, 0}, SwapchainExtent.width, SwapchainExtent.height };
	lightingRenderingInfo.layerCount = 1;
	lightingRenderingInfo.colorAttachmentCount = 1;
	lightingRenderingInfo.pColorAttachments = &lightingColorAttachmentInfo;
	lightingRenderingInfo.pDepthAttachment = nullptr; 
	lightingRenderingInfo.pStencilAttachment = VK_NULL_HANDLE;

	vkCmdBeginRendering(commandBufferCurrentFrame, &lightingRenderingInfo);

	vkCmdBindPipeline(commandBufferCurrentFrame, VK_PIPELINE_BIND_POINT_GRAPHICS, lightingGraphicsPipeline); 
	vkCmdSetViewport(commandBufferCurrentFrame, 0, 1, &viewport); 
	vkCmdSetScissor(commandBufferCurrentFrame, 0, 1, &scissor);

	
	vkCmdBindDescriptorSets(commandBufferCurrentFrame, VK_PIPELINE_BIND_POINT_GRAPHICS, lightingPipelineLayout, 0, 1, &lightingDescriptorSet[currentFrame], 0, nullptr);

	
	ScreenSizePush screenSizePushData;
	screenSizePushData.inverseScreenSize = glm::vec2(1.0f / SwapchainExtent.width, 1.0f / SwapchainExtent.height);
	screenSizePushData.inverseViewProjection = camera->getProjection() * camera->getView(); 

	screenSizePushData.inverseViewProjection = glm::inverse(camera->getProjection() * camera->getView());


	vkCmdPushConstants(
		commandBufferCurrentFrame,
		lightingPipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
		0,
		sizeof(ScreenSizePush),
		&screenSizePushData
	);

	vkCmdDraw(commandBufferCurrentFrame, 3, 1, 0, 0); 
	vkCmdEndRendering(commandBufferCurrentFrame);

	
	Image::RecordImageTransition(commandBufferCurrentFrame,hdrManager->GetHDRResolve(), VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
	Image::RecordImageTransition(commandBufferCurrentFrame,swapchain->GetSwapchainImages()[imageIndex], swapchain->GetSwapChainImageFormat(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);


	VkRenderingAttachmentInfo toneMappingAttachmentInfo{};
	toneMappingAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	toneMappingAttachmentInfo.imageView = swapchain->GetSwapchainImageViews()[imageIndex];
	toneMappingAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	toneMappingAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	toneMappingAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	toneMappingAttachmentInfo.clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

	VkRenderingInfo toneMappingRenderingInfo{};
	toneMappingRenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	toneMappingRenderingInfo.renderArea = VkRect2D{ VkOffset2D {0, 0}, SwapchainExtent.width, SwapchainExtent.height };
	toneMappingRenderingInfo.layerCount = 1;
	toneMappingRenderingInfo.colorAttachmentCount = 1;
	toneMappingRenderingInfo.pColorAttachments = &toneMappingAttachmentInfo;
	toneMappingRenderingInfo.pDepthAttachment = nullptr;
	toneMappingRenderingInfo.pStencilAttachment = nullptr;

	vkCmdBeginRendering(commandBufferCurrentFrame, &toneMappingRenderingInfo);

	vkCmdBindPipeline(commandBufferCurrentFrame, VK_PIPELINE_BIND_POINT_GRAPHICS, HdrGraphicsPipeline);
	vkCmdSetViewport(commandBufferCurrentFrame, 0, 1, &viewport);
	vkCmdSetScissor(commandBufferCurrentFrame, 0, 1, &scissor);

	vkCmdBindDescriptorSets(commandBufferCurrentFrame, VK_PIPELINE_BIND_POINT_GRAPHICS, hdrPipelineLayout, 0, 1, &hdrDescriptorSet[currentFrame], 0, nullptr);
	ToneMapPush tonemapPushData;
	tonemapPushData.exposure = currentExposure;
	tonemapPushData.tonemapOperator = currentTonemapOperator;

	vkCmdPushConstants(
		commandBufferCurrentFrame,
		hdrPipelineLayout,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(ToneMapPush),
		&tonemapPushData
	);

	vkCmdDraw(commandBufferCurrentFrame, 3, 1, 0, 0); 
	vkCmdEndRendering(commandBufferCurrentFrame);

	Image::RecordImageTransition(commandBufferCurrentFrame,swapchain->GetSwapchainImages()[imageIndex], swapchain->GetSwapChainImageFormat(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);

	if (vkEndCommandBuffer(commandBufferCurrentFrame) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void VulkanRenderer::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	VulkanRenderer* renderer = static_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
	if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		renderer->rightMouseButtonPressed = (action == GLFW_PRESS);
		glfwSetInputMode(window, GLFW_CURSOR, renderer->rightMouseButtonPressed ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
		if (renderer->rightMouseButtonPressed)
		{
			renderer->camera->firstMouse = true; 
		}
	}
}