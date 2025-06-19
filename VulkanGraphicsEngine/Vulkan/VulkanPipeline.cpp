#include "VulkanPipeline.h"
#include "VulkanContext.h"
#include "fstream"
#include "Scene.h"

void VulkanPipeline::CleanupPipeline(VkPipeline pipeline, VkPipelineLayout pipelineLayout)
{
    if (context)
    {
        SavePipelineCache();

        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(context->GetDevice(), pipelineLayout, nullptr);
        }

        if (pipelineCache != VK_NULL_HANDLE) {
            vkDestroyPipelineCache(context->GetDevice(), pipelineCache, nullptr);
        }

        if (pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(context->GetDevice(), pipeline, nullptr);
        }
    }
}

VulkanPipeline& VulkanPipeline::DefaultPipelineConfig(PipelineInfo& configInfo, std::vector<VkFormat> formats)
{
    
    configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    // Viewport State
    configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    configInfo.viewportInfo.viewportCount = 1;
    configInfo.viewportInfo.scissorCount = 1;

    // Rasterization State
    configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
    configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    configInfo.rasterizationInfo.lineWidth = 1.0f;
    configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
    configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;

    // Multisample State
    VkSampleCountFlagBits msaaSamples = context->GetMsaaSamples();
    configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    configInfo.multisampleInfo.rasterizationSamples = msaaSamples;

    if (msaaSamples != VK_SAMPLE_COUNT_1_BIT) {
        configInfo.multisampleInfo.sampleShadingEnable = VK_TRUE;
        configInfo.multisampleInfo.minSampleShading = 0.2f;
    }
    else {
        configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
        configInfo.multisampleInfo.minSampleShading = 1.0f;
    }

    //blend attachments
    uint32_t colorAttachmentCount = static_cast<uint32_t>(formats.size());
    configInfo.colorBlendAttachments.resize(colorAttachmentCount);
    for (uint32_t i = 0; i < colorAttachmentCount; ++i) {
        configInfo.colorBlendAttachments[i].colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        configInfo.colorBlendAttachments[i].blendEnable = VK_FALSE;
        configInfo.colorBlendAttachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        configInfo.colorBlendAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        configInfo.colorBlendAttachments[i].colorBlendOp = VK_BLEND_OP_ADD;
        configInfo.colorBlendAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        configInfo.colorBlendAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        configInfo.colorBlendAttachments[i].alphaBlendOp = VK_BLEND_OP_ADD;
    }

    configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
    configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
    configInfo.colorBlendInfo.attachmentCount = colorAttachmentCount;  
    configInfo.colorBlendInfo.pAttachments = configInfo.colorBlendAttachments.data();  
    configInfo.colorBlendInfo.blendConstants[0] = 0.0f;
    configInfo.colorBlendInfo.blendConstants[1] = 0.0f;
    configInfo.colorBlendInfo.blendConstants[2] = 0.0f;
    configInfo.colorBlendInfo.blendConstants[3] = 0.0f;

    // Dynamic State
    configInfo.dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
    configInfo.dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
    configInfo.dynamicStateInfo.flags = 0;

    // Depth Stencil State
    configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
    configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
    configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    configInfo.depthStencilInfo.minDepthBounds = 0.0f;
    configInfo.depthStencilInfo.maxDepthBounds = 1.0f;
    configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
    configInfo.depthStencilInfo.front = {};
    configInfo.depthStencilInfo.back = {};

    // Render pass configuration (legacy)
    configInfo.renderPass = VK_NULL_HANDLE;
    configInfo.subpass = 0;

    // Vertex input configuration
    configInfo.bindingDescriptions = Vertex::GetBindingDescription();
    configInfo.attributeDescriptions = Vertex::GetAttributeDescriptions();

    // Dynamic rendering format configuration
    configInfo.colorAttachmentFormats = formats;
    configInfo.depthAttachmentFormat = VulkanUtils::FindDepthFormat(context->GetPhysicalDevice());
    configInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    return *this;
}

VulkanPipeline& VulkanPipeline::CreatePipelineCache()
{
    // Try to load existing cache first
    pipelineCache = LoadPipelineCache();

    if (pipelineCache == VK_NULL_HANDLE)
    {
        VkPipelineCacheCreateInfo cacheInfo{};
        cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        cacheInfo.initialDataSize = 0;
        cacheInfo.pInitialData = nullptr;

        if (vkCreatePipelineCache(context->GetDevice(), &cacheInfo, nullptr, &pipelineCache) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create pipeline cache!");
        }

        std::cout << "Created new pipeline cache." << std::endl;
    }
    else
    {
        std::cout << "Loaded existing pipeline cache." << std::endl;
    }

    return *this;
}

void VulkanPipeline::SavePipelineCache()
{
    if (pipelineCache == VK_NULL_HANDLE) {
        std::cerr << "Pipeline cache is NULL, skipping save." << std::endl;
        return;
    }

    // Get cache size
    size_t cacheSize = 0;
    VkResult result = vkGetPipelineCacheData(context->GetDevice(), pipelineCache, &cacheSize, nullptr);

    if (result != VK_SUCCESS || cacheSize == 0) {
        std::cerr << "Failed to get pipeline cache data or cache is empty." << std::endl;
        return;
    }

    // Allocate buffer and retrieve cache data
    std::vector<uint8_t> cacheData(cacheSize);
    result = vkGetPipelineCacheData(context->GetDevice(), pipelineCache, &cacheSize, cacheData.data());

    if (result != VK_SUCCESS) {
        std::cerr << "Failed to retrieve pipeline cache data." << std::endl;
        return;
    }

    // Save to file with error handling
    std::ofstream cacheFile("pipeline_cache.bin", std::ios::binary);
    if (!cacheFile.is_open()) {
        std::cerr << "Failed to open pipeline cache file for writing." << std::endl;
        return;
    }

    cacheFile.write(reinterpret_cast<const char*>(cacheData.data()), cacheSize);

    if (!cacheFile.good()) {
        std::cerr << "Error occurred while writing pipeline cache file." << std::endl;
    }
    else {
        std::cout << "Pipeline cache saved successfully (" << cacheSize << " bytes)." << std::endl;
    }

    cacheFile.close();
}

VkPipelineCache VulkanPipeline::LoadPipelineCache()
{
    std::ifstream cacheFile("pipeline_cache.bin", std::ios::binary | std::ios::ate);
    if (!cacheFile.is_open()) {
        return VK_NULL_HANDLE; // File doesn't exist, return null handle
    }

    size_t cacheSize = static_cast<size_t>(cacheFile.tellg());
    if (cacheSize == 0) {
        cacheFile.close();
        return VK_NULL_HANDLE;
    }

    cacheFile.seekg(0, std::ios::beg);

    std::vector<uint8_t> cacheData(cacheSize);
    cacheFile.read(reinterpret_cast<char*>(cacheData.data()), cacheSize);

    if (!cacheFile.good()) {
        std::cerr << "Error reading pipeline cache file." << std::endl;
        cacheFile.close();
        return VK_NULL_HANDLE;
    }

    cacheFile.close();

    VkPipelineCacheCreateInfo cacheInfo{};
    cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    cacheInfo.initialDataSize = cacheSize;
    cacheInfo.pInitialData = cacheData.data();

    VkPipelineCache loadedCache;
    VkResult result = vkCreatePipelineCache(context->GetDevice(), &cacheInfo, nullptr, &loadedCache);

    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create pipeline cache from loaded data. Creating new cache." << std::endl;
        return VK_NULL_HANDLE;
    }

    return loadedCache;
}

VkShaderModule VulkanPipeline::CreateShaderModule(VkDevice device, const std::vector<char>& code)
{
    if (code.empty()) {
        throw std::runtime_error("Shader code is empty!");
    }

    // Ensure code size is properly aligned for uint32_t
    if (code.size() % sizeof(uint32_t) != 0) {
        throw std::runtime_error("Shader code size is not properly aligned!");
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}