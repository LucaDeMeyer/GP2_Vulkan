#ifndef VULKAN_PIPELINE_H
#define VULKAN_PIPELINE_H
#include "VulkanUtils.h"
#include "VulkanContext.h"
class VulkanContext;

struct PipelineInfo
{
    PipelineInfo() = default;
    PipelineInfo(const PipelineInfo&) = delete;
    PipelineInfo& operator=(const PipelineInfo&) = delete;

    std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
    VkPipelineViewportStateCreateInfo viewportInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationInfo;
    VkPipelineMultisampleStateCreateInfo multisampleInfo;

    // CHANGED: Support multiple color blend attachments
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments{};
    VkPipelineColorBlendStateCreateInfo colorBlendInfo;

    VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
    std::vector<VkDynamicState> dynamicStateEnables;
    VkPipelineDynamicStateCreateInfo dynamicStateInfo;
    VkPipelineLayout pipelineLayout = nullptr;
    VkRenderPass renderPass = nullptr;
    uint32_t subpass = 0;

   
    std::vector<VkFormat> colorAttachmentFormats;
    uint32_t colorAttachmentCount = static_cast<uint32_t>(colorAttachmentFormats.size());
    VkFormat depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    VkFormat stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
	
};

class VulkanPipeline final 
{
public:

	VulkanPipeline(VulkanContext* context) : context(context) {} 
	~VulkanPipeline() = default;

	VkRenderPass GetRenderPass() const { return renderPass; }
	//VkPipeline GetGraphicsPipeline() const { return graphicsPipeline; }
	//VkPipelineLayout GetPipelineLayout() const { return pipelineLayout; }

	void CleanupPipeline(VkPipeline pipeline, VkPipelineLayout pipelineLayout);

    template<typename TPushConstant = void>
    VulkanPipeline& CreateGraphicsPipeline(
        const std::string& vertShaderFilePath,
        const std::string& fragShaderFilePath,
        PipelineInfo& pipelineConfigInfo,
        VkDescriptorSetLayout descriptorSetLayout,
        VkPipeline& pipeline,
        VkPipelineLayout& pipelineLayout,
        VkShaderStageFlags pushConstantStageFlags = 0 
    )
    {
        // IMPORTANT: The entire definition of CreateGraphicsPipeline
        // (the code you provided in the question) goes here.

        // Validate inputs
        if (vertShaderFilePath.empty() || fragShaderFilePath.empty()) {
            throw std::runtime_error("Shader file paths cannot be empty!");
        }

        if (descriptorSetLayout == VK_NULL_HANDLE) {
            throw std::runtime_error("Descriptor set layout cannot be VK_NULL_HANDLE!");
        }

        auto vertShaderCode = VulkanUtils::ReadFile(vertShaderFilePath);
        auto fragShaderCode = VulkanUtils::ReadFile(fragShaderFilePath);

        VkShaderModule vertShaderModule = CreateShaderModule(context->GetDevice(), vertShaderCode);
        VkShaderModule fragShaderModule = CreateShaderModule(context->GetDevice(), fragShaderCode);

        // RAII-style cleanup for shader modules
        auto cleanupShaders = [&]() {
            vkDestroyShaderModule(context->GetDevice(), fragShaderModule, nullptr);
            vkDestroyShaderModule(context->GetDevice(), vertShaderModule, nullptr);
            };

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = pipelineConfigInfo.bindingDescriptions;
        auto attributeDescriptions = pipelineConfigInfo.attributeDescriptions;

        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescription.size());
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = bindingDescription.data();
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

        // --- Push Constant Handling ---
        VkPushConstantRange pushConstantRange{};
        if (pushConstantStageFlags != 0) {
            pushConstantRange.offset = 0;
            // Handle void specialization:
            if constexpr (!std::is_same_v<TPushConstant, void>) {
                pushConstantRange.size = sizeof(TPushConstant);
            }
            else {
                pushConstantRange.size = 0; // Or throw an error if void with stageFlags is invalid
            }
            pushConstantRange.stageFlags = pushConstantStageFlags;
            pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
            pipelineLayoutInfo.pushConstantRangeCount = 1;
        }
        else {
            pipelineLayoutInfo.pPushConstantRanges = nullptr;
            pipelineLayoutInfo.pushConstantRangeCount = 0;
        }
        // --- End Push Constant Handling ---

        if (vkCreatePipelineLayout(context->GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            cleanupShaders();
            throw std::runtime_error("failed to create pipeline layout!");
        }

        pipelineConfigInfo.pipelineLayout = pipelineLayout;

        // Dynamic rendering setup
        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        renderingInfo.colorAttachmentCount = pipelineConfigInfo.colorAttachmentCount;


        if (pipelineConfigInfo.colorAttachmentCount == 1) {
            renderingInfo.pColorAttachmentFormats = &pipelineConfigInfo.colorAttachmentFormats[0];
        }
        else {
            renderingInfo.pColorAttachmentFormats = pipelineConfigInfo.colorAttachmentFormats.data();
        }

		

        renderingInfo.depthAttachmentFormat = pipelineConfigInfo.depthAttachmentFormat;
        renderingInfo.viewMask = 0;
        renderingInfo.pNext = nullptr;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &pipelineConfigInfo.inputAssemblyInfo;
        pipelineInfo.pViewportState = &pipelineConfigInfo.viewportInfo;
        pipelineInfo.pRasterizationState = &pipelineConfigInfo.rasterizationInfo;
        pipelineInfo.pMultisampleState = &pipelineConfigInfo.multisampleInfo;
        pipelineInfo.pColorBlendState = &pipelineConfigInfo.colorBlendInfo;
        pipelineInfo.pDepthStencilState = &pipelineConfigInfo.depthStencilInfo;
        pipelineInfo.pDynamicState = &pipelineConfigInfo.dynamicStateInfo;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = VK_NULL_HANDLE; // Using dynamic rendering
        pipelineInfo.subpass = 0;
        pipelineInfo.pNext = &renderingInfo;

        VkResult result = vkCreateGraphicsPipelines(context->GetDevice(), pipelineCache, 1, &pipelineInfo, nullptr, &pipeline);

        // Clean up shader modules regardless of success or failure
        cleanupShaders();
     
        if (result != VK_SUCCESS) {
            // Clean up pipeline layout on failure
            vkDestroyPipelineLayout(context->GetDevice(), pipelineLayout, nullptr);
            pipelineLayout = VK_NULL_HANDLE;
            throw std::runtime_error("failed to create graphics pipeline!");
        }

       

        return *this;
    }


	VulkanPipeline& CreatePipelineCache();

	VulkanPipeline& DefaultPipelineConfig(PipelineInfo& configInfo,  std::vector<VkFormat> formats);

private:

	void SavePipelineCache();
	VkPipelineCache LoadPipelineCache();

	VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code);

	VulkanContext* context;


	VkRenderPass renderPass;

	VkPipelineCache pipelineCache;

};
#endif