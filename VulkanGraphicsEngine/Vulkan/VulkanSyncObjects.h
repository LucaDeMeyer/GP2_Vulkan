#ifndef VULKAN_SYNC_OBJECTS_H
#define VULKAN_SYNC_OBJECTS_H
#include "VulkanUtils.h"

class VulkanContext;
class VulkanSyncObjects
{
public:
    VulkanSyncObjects(VulkanContext* context) : context(context) {}
    ~VulkanSyncObjects() = default;

    void CreateSyncObjects();

    void CleanupSyncObjects();

    VkSemaphore GetImageAvailableSemaphore(uint32_t frameIndex) { return imageAvailableSemaphores[frameIndex]; }
    VkSemaphore GetRenderFinishedSemaphore(uint32_t frameIndex) { return renderFinishedSemaphores[frameIndex]; }
     VkFence GetInFlightFence(uint32_t frameIndex) { return inFlightFences[frameIndex]; }

private:
    VulkanContext* context;


    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
};



#endif