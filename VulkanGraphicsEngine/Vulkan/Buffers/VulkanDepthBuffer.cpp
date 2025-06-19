#include "VulkanDepthBuffer.h"
#include "VulkanContext.h"
#include "Image.h"


void VulkanDepthBuffer::CleanupDepthBuffer()
{
	if(context)
	{
		vkDestroyImageView(context->GetDevice(), depthImageView, nullptr);
		vmaDestroyImage(context->GetVMAAllocator(), depthImage, depthImageAllocation);
		vmaDestroyImage(context->GetVMAAllocator(), DepthResolveImage, depthImageAllocation);
	}
}

void VulkanDepthBuffer::CreateDepthResources(VkExtent2D swapchainExtent)
{
	VkFormat depthFormat = VulkanUtils::FindDepthFormat(context->GetPhysicalDevice());

	Image::CreateImage(context->GetVMAAllocator(), swapchainExtent.width, swapchainExtent.height, depthFormat,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, depthImage,1, context->GetMsaaSamples(), depthImageAllocation);

	depthImageView = Image::CreateImageView(context->GetDevice(), depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT,1);


	Image::CreateImage(context->GetVMAAllocator(), swapchainExtent.width, swapchainExtent.height, depthFormat,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, DepthResolveImage, 1, VK_SAMPLE_COUNT_1_BIT, depthResolveImageAllocation);
	depthResolveImageView = Image::CreateImageView(context->GetDevice(), DepthResolveImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

}
