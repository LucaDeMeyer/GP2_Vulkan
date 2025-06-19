#include "HDRManager.h"
#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanPipeline.h"
#include "VulkanDescriptorManager.h"
#include "VulkanUtils.h"
#include "Image.h"

HDRManager::HDRManager(VulkanContext* context, VulkanSwapchain* swapchain,
    VulkanPipeline* pipeline, VulkanDescriptorManager* descriptorManager)
    : context(context), swapchain(swapchain), pipeline(pipeline), descriptorManager(descriptorManager) {
}

HDRManager::~HDRManager() {
    Cleanup();
}

void HDRManager::Initialize() {
    CreateHDRResources();
}

void HDRManager::Cleanup() {
	if (hdrSampler != VK_NULL_HANDLE) {
		vkDestroySampler(context->GetDevice(), hdrSampler, nullptr);
		hdrSampler = VK_NULL_HANDLE;
	}

	if (hdrResolveImageView != VK_NULL_HANDLE) {
		vkDestroyImageView(context->GetDevice(), hdrResolveImageView, nullptr);
		hdrResolveImageView = VK_NULL_HANDLE;
	}
	if (hdrResolveImage != VK_NULL_HANDLE && hdrResolveImageMemory != VK_NULL_HANDLE) {
		vmaDestroyImage(context->GetVMAAllocator(), hdrResolveImage, hdrResolveImageMemory);
		hdrResolveImage = VK_NULL_HANDLE;
		hdrResolveImageMemory = VK_NULL_HANDLE;
	}

	if (hdrMsaaView != VK_NULL_HANDLE) {
		vkDestroyImageView(context->GetDevice(), hdrMsaaView, nullptr);
		hdrMsaaView = VK_NULL_HANDLE;
	}
	if (hdrMsaaImage != VK_NULL_HANDLE && hdrMsaaImageMemory != VK_NULL_HANDLE) {
		vmaDestroyImage(context->GetVMAAllocator(), hdrMsaaImage, hdrMsaaImageMemory);
		hdrMsaaImage = VK_NULL_HANDLE;
		hdrMsaaImageMemory = VK_NULL_HANDLE;
	}
}


void HDRManager::CreateHDRResources() {
    VkExtent2D extent = swapchain->GetSwapChainExtent();

    VkSampleCountFlagBits msaaSamples = context->GetMsaaSamples(); 


	Image::CreateImage(context->GetVMAAllocator(), extent.width, extent.height, VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		hdrResolveImage, 1, VK_SAMPLE_COUNT_1_BIT, hdrResolveImageMemory);

	Image::CreateImage(context->GetVMAAllocator(), extent.width, extent.height, VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		hdrMsaaImage, 1, msaaSamples, hdrMsaaImageMemory);

    hdrResolveImageView = Image::CreateImageView(context->GetDevice(), hdrResolveImage, VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT, 1);

    hdrMsaaView = Image::CreateImageView(context->GetDevice(), hdrMsaaImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    // Create sampler for HDR texture
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(context->GetDevice(), &samplerInfo, nullptr, &hdrSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create HDR sampler!");
    }
}
 