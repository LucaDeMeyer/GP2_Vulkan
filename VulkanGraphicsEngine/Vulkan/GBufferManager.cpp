#include "GBufferManager.h"
#include "Image.h"
GBufferManager::GBufferManager(VulkanContext* context) : context{context}
{ }

GBufferManager::~GBufferManager()
{
	CleanupGBuffer();
}

void GBufferManager::Initialize(VkExtent2D swapChainExtent,uint32_t miplevels)
{
	albedoFormat = VK_FORMAT_R8G8B8A8_UNORM; 
	aoFormat = VK_FORMAT_R8_UNORM;           
	normalFormat = VK_FORMAT_R16G16B16A16_SFLOAT; 
	metallicRoughnessFormat = VK_FORMAT_R8G8_UNORM; 


	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(context->GetPhysicalDevice(), &properties);

	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(miplevels);

	if (vkCreateSampler(context->GetDevice(), &samplerInfo, nullptr, &gBufferSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}

}



void GBufferManager::CreateGBufferResources(VkExtent2D extent)
{
	Image::CreateImage(context->GetVMAAllocator(), extent.width, extent.height, albedoFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, albedoImage, 1, context->GetMsaaSamples(), albedoAllocation);
	albedoImageView = Image::CreateImageView(context->GetDevice(), albedoImage, albedoFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	Image::CreateImage(context->GetVMAAllocator(), extent.width, extent.height, aoFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, aoImage, 1, context->GetMsaaSamples(), aoAllocation);
	aoImageView = Image::CreateImageView(context->GetDevice(), aoImage, aoFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	Image::CreateImage(context->GetVMAAllocator(), extent.width, extent.height, normalFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, normalImage, 1, context->GetMsaaSamples(), normalAllocation);
	normalImageView = Image::CreateImageView(context->GetDevice(), normalImage, normalFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	Image::CreateImage(context->GetVMAAllocator(), extent.width, extent.height, metallicRoughnessFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, metallicRoughnessImage, 1, context->GetMsaaSamples(), metallicRoughnessAllocation);
	metallicRoughnessImageView = Image::CreateImageView(context->GetDevice(), metallicRoughnessImage, metallicRoughnessFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	Image::CreateImage(context->GetVMAAllocator(), extent.width, extent.height,albedoFormat,VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,AlbedoImageResolve,1,VK_SAMPLE_COUNT_1_BIT,albedoImageResolveAllocation );
	albedoImageResolveView = Image::CreateImageView(context->GetDevice(), AlbedoImageResolve, albedoFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	Image::CreateImage(context->GetVMAAllocator(), extent.width, extent.height, aoFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, aoImageResolve, 1, VK_SAMPLE_COUNT_1_BIT, aoImageResolveAllocation);
	aoImageResolveView = Image::CreateImageView(context->GetDevice(), aoImageResolve, aoFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	Image::CreateImage(context->GetVMAAllocator(), extent.width, extent.height, normalFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, normalImageResolve, 1, VK_SAMPLE_COUNT_1_BIT, normalImageResolveAllocation);
	normalImageResolveView = Image::CreateImageView(context->GetDevice(), normalImageResolve, normalFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	Image::CreateImage(context->GetVMAAllocator(), extent.width, extent.height, metallicRoughnessFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, metallicRoughnessImageResolve, 1, VK_SAMPLE_COUNT_1_BIT, metallicRoughnessImageResolveAllocation);
	metallicRoughnessImageResolveView = Image::CreateImageView(context->GetDevice(), metallicRoughnessImageResolve, metallicRoughnessFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	Image::CreateImage(context->GetVMAAllocator(), extent.width, extent.height, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, gWorldPosImage, 1, context->GetMsaaSamples(), gWorldPosImageAllocation);
	gWorldPosImageView = Image::CreateImageView(context->GetDevice(), gWorldPosImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	Image::CreateImage(context->GetVMAAllocator(), extent.width, extent.height, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, gWorldPosResolveImage, 1, VK_SAMPLE_COUNT_1_BIT, gWorldPosImageResolveAllocation);
	gWorldPosResolveImageView = Image::CreateImageView(context->GetDevice(), gWorldPosResolveImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1);

}

void GBufferManager::CleanupGBuffer()
{
	// Destroy the sampler first
	if (gBufferSampler != VK_NULL_HANDLE) {
		vkDestroySampler(context->GetDevice(), gBufferSampler, nullptr);
		gBufferSampler = VK_NULL_HANDLE; // Set to null to prevent double-free
	}

	// Destroy image views and images with VMA
	if (metallicRoughnessImageView != VK_NULL_HANDLE) {
		vkDestroyImageView(context->GetDevice(), metallicRoughnessImageView, nullptr);
		metallicRoughnessImageView = VK_NULL_HANDLE;
	}
	if (metallicRoughnessImage != VK_NULL_HANDLE && metallicRoughnessAllocation != VK_NULL_HANDLE) {
		vmaDestroyImage(context->GetVMAAllocator(), metallicRoughnessImage, metallicRoughnessAllocation);
		metallicRoughnessImage = VK_NULL_HANDLE;
		metallicRoughnessAllocation = VK_NULL_HANDLE;
	}

	if (normalImageView != VK_NULL_HANDLE) {
		vkDestroyImageView(context->GetDevice(), normalImageView, nullptr);
		normalImageView = VK_NULL_HANDLE;
	}
	if (normalImage != VK_NULL_HANDLE && normalAllocation != VK_NULL_HANDLE) {
		vmaDestroyImage(context->GetVMAAllocator(), normalImage, normalAllocation);
		normalImage = VK_NULL_HANDLE;
		normalAllocation = VK_NULL_HANDLE;
	}

	if (aoImageView != VK_NULL_HANDLE) {
		vkDestroyImageView(context->GetDevice(), aoImageView, nullptr);
		aoImageView = VK_NULL_HANDLE;
	}
	if (aoImage != VK_NULL_HANDLE && aoAllocation != VK_NULL_HANDLE) {
		vmaDestroyImage(context->GetVMAAllocator(), aoImage, aoAllocation);
		aoImage = VK_NULL_HANDLE;
		aoAllocation = VK_NULL_HANDLE;
	}

	if (albedoImageView != VK_NULL_HANDLE) {
		vkDestroyImageView(context->GetDevice(), albedoImageView, nullptr);
		albedoImageView = VK_NULL_HANDLE;
	}
	if (albedoImage != VK_NULL_HANDLE && albedoAllocation != VK_NULL_HANDLE) {
		vmaDestroyImage(context->GetVMAAllocator(), albedoImage, albedoAllocation);
		albedoImage = VK_NULL_HANDLE;
		albedoAllocation = VK_NULL_HANDLE;
	}

	vkDestroyImageView(context->GetDevice(), gWorldPosImageView, nullptr);
	vmaDestroyImage(context->GetVMAAllocator(), gWorldPosImage, gWorldPosImageAllocation);

	vkDestroyImageView(context->GetDevice(), gWorldPosResolveImageView, nullptr);
	vmaDestroyImage(context->GetVMAAllocator(), gWorldPosResolveImage, gWorldPosImageResolveAllocation);
}