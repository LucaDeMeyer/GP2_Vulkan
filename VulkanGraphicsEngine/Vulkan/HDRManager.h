#ifndef HDRMANAGER_H
#define HDRMANAGER_H

#include "VulkanUtils.h"
class VulkanContext;
class VulkanSwapchain;
class VulkanPipeline;
class VulkanDescriptorManager;

class HDRManager {
public:
    enum class TonemapOperator {
        REINHARD = 0,
        ACES = 1,
        UNCHARTED2 = 2
    };

    struct TonemapParams {
        float exposure = 1.0f;
        TonemapOperator tonemapOperator = TonemapOperator::ACES;
    };

    HDRManager(VulkanContext* context, VulkanSwapchain* swapchain,
        VulkanPipeline* pipeline, VulkanDescriptorManager* descriptorManager);
    ~HDRManager();

    void Initialize();
    void Cleanup();


    // Getters for rendering
    VkImage GetHDRMsaa() const { return hdrMsaaImage; }
    VkImageView GetHDRMsaaView() const { return hdrMsaaView; }
	VkImage GetHDRResolve() const { return hdrResolveImage; }
	VkImageView GetHDRResolveView() const { return hdrResolveImageView; }
	VkSampler GetHDRSampler() const { return hdrSampler; }

    
private:
    VulkanContext* context;
    VulkanSwapchain* swapchain;
    VulkanPipeline* pipeline;
    VulkanDescriptorManager* descriptorManager;

    // HDR resources
    VkImage hdrMsaaImage = VK_NULL_HANDLE;
    VkImage hdrResolveImage = VK_NULL_HANDLE;
    VmaAllocation hdrMsaaImageMemory = VK_NULL_HANDLE;
    VmaAllocation hdrResolveImageMemory = VK_NULL_HANDLE;
    VkImageView hdrMsaaView = VK_NULL_HANDLE;
	VkImageView hdrResolveImageView = VK_NULL_HANDLE;
    VkSampler hdrSampler = VK_NULL_HANDLE;

    void CreateHDRResources();;
};





#endif