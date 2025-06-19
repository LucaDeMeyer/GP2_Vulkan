#pragma once
#include "VulkanUtils.h"
#include <vector>
#include "VulkanContext.h" 

class GBufferManager {
public:
    // Constructor now takes VmaAllocator
    GBufferManager(VulkanContext* context);
    ~GBufferManager();

    void Initialize(VkExtent2D swapChainExtent, uint32_t miplevels);
    void CreateGBufferResources(VkExtent2D extent);
    void CleanupGBuffer();

    // Getters for G-Buffer image views and formats
    VkImageView GetAlbedoImageView() const { return albedoImageView; }
    VkImageView GetAOImageView() const { return aoImageView; } 
    VkImageView GetNormalImageView() const { return normalImageView; }
    VkImageView GetMetallicRoughnessImageView() const { return metallicRoughnessImageView; }

    VkImage GetAlbedoImage() const { return albedoImage; }
    VkImage GetAOImage() const { return aoImage; } 
    VkImage GetNormalImage() const { return normalImage; }
    VkImage GetMetallicRoughnessImage() const { return metallicRoughnessImage; }

	VkImage GetAlbedoImageResolve() const { return AlbedoImageResolve; }
	VkImage GetAOImageResolve() const { return aoImageResolve; }
	VkImage GetNormalImageResolve() const { return normalImageResolve; }
	VkImage GetMetallicRoughnessImageResolve() const { return metallicRoughnessImageResolve; }

    VkFormat GetAlbedoImageFormat() const { return albedoFormat; }
    VkFormat GetAOImageFormat() const { return aoFormat; } 
    VkFormat GetNormalImageFormat() const { return normalFormat; }
    VkFormat GetMetallicRoughnessImageFormat() const { return metallicRoughnessFormat; }

	VkImageView GetAlbedoImageResolveView() const { return albedoImageResolveView; }
	VkImageView GetAOImageResolveView() const { return aoImageResolveView; }
	VkImageView GetNormalImageResolveView() const { return normalImageResolveView; }
	VkImageView GetMetallicRoughnessImageResolveView() const { return metallicRoughnessImageResolveView; }

	VkImageView GetGWorldPosImageView() const { return gWorldPosImageView; }
	VkImageView GetGWorldPosResolveImageView() const { return gWorldPosResolveImageView; }
	VkImage GetGWorldPosImage() const { return gWorldPosImage; }
	VkImage GetGWorldPosResolveImage() const { return gWorldPosResolveImage; }

    VkSampler GetGBufferSampler() const { return gBufferSampler; }

	VkImageLayout GetAlbedoImageLayout() const { return albedoImageLayout; }
	VkImageLayout GetAOImageLayout() const { return aoImageLayout; }
	VkImageLayout GetNormalImageLayout() const { return normalImageLayout; }
	VkImageLayout GetMetallicRoughnessImageLayout() const { return metallicRoughnessImageLayout; }

	VkImageLayout GetGWorldPosImageLayout() const { return gWorldPosImageLayout; }
	VkImageLayout GetGWorldPosResolveImageLayout() const { return gWorldPosResolveImageLayout; }

	VkImageLayout GetAlbedoImageResolveLayout() const { return albedoImageResolveLayout; }
	VkImageLayout GetAOImageResolveLayout() const { return aoImageResolveLayout; }
	VkImageLayout GetNormalImageResolveLayout() const { return normalImageResolveLayout; }
	VkImageLayout GetMetallicRoughnessImageResolveLayout() const { return metallicRoughnessImageResolveLayout; }

	void SetGWorldPosImageLayout(VkImageLayout layout) { gWorldPosImageLayout = layout; }
	void SetGWorldPosResolveImageLayout(VkImageLayout layout) { gWorldPosResolveImageLayout = layout; }

	void SetAlbedoImageLayout(VkImageLayout layout) { albedoImageLayout = layout; }
	void SetAOImageLayout(VkImageLayout layout) { aoImageLayout = layout; }
	void SetNormalImageLayout(VkImageLayout layout) { normalImageLayout = layout; }
	void SetMetallicRoughnessImageLayout(VkImageLayout layout) { metallicRoughnessImageLayout = layout; }

	void SetAlbedoImageResolveLayout(VkImageLayout layout) { albedoImageResolveLayout = layout; }
	void SetAOImageResolveLayout(VkImageLayout layout) { aoImageResolveLayout = layout; }
	void SetNormalImageResolveLayout(VkImageLayout layout) { normalImageResolveLayout = layout; }
	void SetMetallicRoughnessImageResolveLayout(VkImageLayout layout) { metallicRoughnessImageResolveLayout = layout; }

private:
    VulkanContext* context;
  
	VkImageLayout albedoImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout aoImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout normalImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout metallicRoughnessImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkImageLayout gWorldPosImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout gWorldPosResolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkImageLayout albedoImageResolveLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout aoImageResolveLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout normalImageResolveLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout metallicRoughnessImageResolveLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    
    VkImage albedoImage;
    VmaAllocation albedoAllocation;
    VkImageView albedoImageView;
    VkFormat albedoFormat;

	VkImage AlbedoImageResolve;
	VmaAllocation albedoImageResolveAllocation;
	VkImageView albedoImageResolveView;
  
    VkImage aoImage;
    VmaAllocation aoAllocation;
    VkImageView aoImageView;
    VkFormat aoFormat;
	VkImage aoImageResolve;
	VmaAllocation aoImageResolveAllocation;
	VkImageView aoImageResolveView;

    VkImage normalImage;
    VmaAllocation normalAllocation;
    VkImageView normalImageView;
    VkFormat normalFormat;
	VkImage normalImageResolve;
	VmaAllocation normalImageResolveAllocation;
	VkImageView normalImageResolveView;
   
    VkImage metallicRoughnessImage;
    VmaAllocation metallicRoughnessAllocation;
    VkImageView metallicRoughnessImageView;
    VkFormat metallicRoughnessFormat;
	VkImage metallicRoughnessImageResolve;
	VmaAllocation metallicRoughnessImageResolveAllocation;
	VkImageView metallicRoughnessImageResolveView;

    VkImage gWorldPosImage;
	VkImage gWorldPosResolveImage;
	VmaAllocation gWorldPosImageAllocation;
	VkImageView gWorldPosImageView;
	VkImageView gWorldPosResolveImageView;
	VmaAllocation gWorldPosImageResolveAllocation;

    VkSampler gBufferSampler;
};
