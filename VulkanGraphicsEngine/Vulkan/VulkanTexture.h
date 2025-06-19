#ifndef VULKAN_TEXTURE_H
#define VULKAN_TEXTURE_H
#include "VulkanUtils.h"


class VulkanContext;
class VulkanTexture
{
public:

	VulkanTexture(VulkanContext* context) : context(context) , textureImage(VK_NULL_HANDLE),textureImageView(VK_NULL_HANDLE),textureSampler(VK_NULL_HANDLE),mipLevels(0),textureImageAllocation(nullptr){}
    ~VulkanTexture();
   
	VulkanTexture(const VulkanTexture&) = delete;
	VulkanTexture& operator=(const VulkanTexture&) = delete;


    // --- MOVE CONSTRUCTOR ---
    VulkanTexture(VulkanTexture&& other) noexcept :
        context(other.context), // Context pointer can be copied
        textureImage(other.textureImage),
        textureImageView(other.textureImageView),
        textureSampler(other.textureSampler),
        mipLevels(other.mipLevels),
        textureImageAllocation(other.textureImageAllocation)
    {
        other.textureImage = VK_NULL_HANDLE;
        other.textureImageView = VK_NULL_HANDLE;
        other.textureSampler = VK_NULL_HANDLE;
        other.mipLevels = 0; // Or appropriate default
        other.textureImageAllocation = VK_NULL_HANDLE;
    }

    // --- MOVE ASSIGNMENT OPERATOR ---
    VulkanTexture& operator=(VulkanTexture&& other) noexcept
    {
        if (this != &other) 
        {
            // 1. Clean up *this* object's current resources
            CleanupTexture(); 
            // 2. Transfer ownership from 'other' to *this*
            context = other.context; // Context pointer can be copied
            textureImage = other.textureImage;
            textureImageView = other.textureImageView;
            textureSampler = other.textureSampler;
            mipLevels = other.mipLevels;
            textureImageAllocation = other.textureImageAllocation;
            // 3. Nullify 'other' object's handles
            other.textureImage = VK_NULL_HANDLE;
            other.textureImageView = VK_NULL_HANDLE;
            other.textureSampler = VK_NULL_HANDLE;
            other.mipLevels = 0;
            other.textureImageAllocation = VK_NULL_HANDLE;
        }
        return *this;
    }
	VkImageView GetTextureImageView() const { return textureImageView; }
	VkImage GetTextureImage() const { return textureImage; }
	VkSampler GetTextureSampler() const { return textureSampler; }




	VulkanTexture& CreateTexture(const std::string& texturePath,TextureType type);

	void CleanupTexture();
	 
private:

	void GenerateMipMaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t miplevels);
	VulkanTexture& CreateTextureSampler();
	VulkanTexture& CreateTextureImageView(TextureType type);

	VulkanContext* context;

	VkImage textureImage;
	VkImageView textureImageView;


	VkSampler textureSampler;
	uint32_t mipLevels;

	VmaAllocation textureImageAllocation;
    VmaAllocation textureResolveAllocation;

};
#endif
