#include "VulkanTexture.h"
#include "VulkanContext.h"
#include "Image.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include <contrib/stb/stb_image.h>


VulkanTexture::~VulkanTexture()
{
	CleanupTexture();
}

void VulkanTexture::CleanupTexture()
{
	if (context)
	{
		vkDestroySampler(context->GetDevice(), textureSampler, nullptr);
		vkDestroyImageView(context->GetDevice(), textureImageView, nullptr);
		vmaDestroyImage(context->GetVMAAllocator(), textureImage, textureImageAllocation);

	}

	textureSampler = VK_NULL_HANDLE;
	textureImageView = VK_NULL_HANDLE;
	textureImage = VK_NULL_HANDLE;
	textureImageAllocation = nullptr; 
	mipLevels = 0; 
}

VulkanTexture& VulkanTexture::CreateTextureSampler()
{
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
	samplerInfo.maxLod = static_cast<float>(mipLevels);

	if (vkCreateSampler(context->GetDevice(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}

	return *this;
}

VulkanTexture& VulkanTexture::CreateTextureImageView(TextureType type)
{
	if (type == TextureType::NORMAL)
	{
		textureImageView = Image::CreateImageView(context->GetDevice(), textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
	}
	else
	{
		textureImageView = Image::CreateImageView(context->GetDevice(), textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
	}
	return *this;
}


VulkanTexture& VulkanTexture::CreateTexture(const std::string& texturePath,TextureType type)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}
	mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	VkBuffer stagingBuffer;
	
	VmaAllocation stagingAlloc{};
	

	VulkanUtils::CreateBuffer(context->GetVMAAllocator(),imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer, VMA_MEMORY_USAGE_CPU_ONLY,stagingAlloc);

	void* data;
	vmaMapMemory(context->GetVMAAllocator(), stagingAlloc, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vmaUnmapMemory(context->GetVMAAllocator(), stagingAlloc);

	stbi_image_free(pixels);

	if (type != TextureType::NORMAL)
	{
		Image::CreateImage(context->GetVMAAllocator(), texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, textureImage, mipLevels, VK_SAMPLE_COUNT_1_BIT, textureImageAllocation);
	}
	else
	{
		Image::CreateImage(context->GetVMAAllocator(), texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, textureImage, mipLevels, VK_SAMPLE_COUNT_1_BIT, textureImageAllocation);
	}

	if (type == TextureType::NORMAL)
	Image::TransitionImageLayout(context->GetDevice(),context->GetCommandPool(),context->GetGraphicsQueue(), textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,mipLevels);
	else
		Image::TransitionImageLayout(context->GetDevice(), context->GetCommandPool(), context->GetGraphicsQueue(), textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

	Image::CopyBufferToImage(context->GetDevice(), context->GetCommandPool(), context->GetGraphicsQueue(), textureImage, stagingBuffer, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

	vmaDestroyBuffer(context->GetVMAAllocator(), stagingBuffer, stagingAlloc);

	if (type == TextureType::NORMAL)
		GenerateMipMaps(textureImage,VK_FORMAT_R8G8B8A8_UNORM, texWidth, texHeight,mipLevels);
	else
		GenerateMipMaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);

	CreateTextureImageView(type);

	CreateTextureSampler();

	return *this;
}

void VulkanTexture::GenerateMipMaps(VkImage image,VkFormat imageFormat,int32_t texWidth, int32_t texHeight,uint32_t miplevels)
{

	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(context->GetPhysicalDevice(), imageFormat, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	VkCommandBuffer commandBuffer = VulkanUtils::BeginSingleTimeCommands(context->GetDevice(),context->GetCommandPool());

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	
	for (uint32_t i = 1; i < miplevels; i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
		// Wait for the blit operation to complete before transitioning
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);
	}

		barrier.subresourceRange.baseMipLevel = miplevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);
	

	VulkanUtils::EndSingleTimeCommands(context->GetDevice(),context->GetGraphicsQueue(),commandBuffer,context->GetCommandPool());
}


