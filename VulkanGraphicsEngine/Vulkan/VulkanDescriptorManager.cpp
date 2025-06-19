#include "VulkanDescriptorManager.h"

#include "Scene.h"
#include "VulkanContext.h"

VulkanDescriptorManager::VulkanDescriptorManager(VulkanContext* ctx) : context(ctx) {
    if (!context) {
        throw std::runtime_error("VulkanContext cannot be null for VulkanDescriptorManager");
    }
}

void VulkanDescriptorManager::CreateDescriptorPool(
    const std::vector<VkDescriptorPoolSize>& poolSizes,
    uint32_t maxSets,
    VkDescriptorPoolCreateFlags flags)
{
    if (descriptorPool != VK_NULL_HANDLE) {
        throw std::runtime_error("Descriptor pool already created. Call CleanupPool first.");
    }
    if (poolSizes.empty() || maxSets == 0) {
        throw std::runtime_error("Invalid pool sizes or max sets for descriptor pool creation.");
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = flags;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxSets;

    if (vkCreateDescriptorPool(context->GetDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}

VkDescriptorSetLayout VulkanDescriptorManager::CreateDescriptorSetLayout(
    const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
    if (bindings.empty()) {
       throw std::runtime_error("Cannot create a descriptor set layout with zero bindings.");
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();


    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    if (vkCreateDescriptorSetLayout(context->GetDevice(), &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }

    return layout; // Return the handle
}


std::vector<VkDescriptorSet> VulkanDescriptorManager::AllocateAndWriteDescriptorSets(
    VkDescriptorSetLayout layout,
    uint32_t setCount,
    std::function<std::pair<std::vector<DescriptorBufferBinding>, std::vector<DescriptorImageBinding>>(uint32_t setIndex)> getBindingsForSet)
{
    if (descriptorPool == VK_NULL_HANDLE) {
        throw std::runtime_error("Descriptor pool has not been created!");
    }
    if (layout == VK_NULL_HANDLE) {
        throw std::runtime_error("Cannot allocate descriptor sets with a null layout!");
    }
    if (setCount == 0) {
        return {}; 
    }
    if (!getBindingsForSet) {
        throw std::runtime_error("Binding provider callback is null!");
    }

    // 1. Allocate Descriptor Sets
    std::vector<VkDescriptorSetLayout> layouts(setCount, layout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = setCount;
    allocInfo.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSet> allocatedSets(setCount);
    VkResult result = vkAllocateDescriptorSets(context->GetDevice(), &allocInfo, allocatedSets.data());
    if (result != VK_SUCCESS) {
        // Provide more context on pool exhaustion if possible
        if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
            throw std::runtime_error("Failed to allocate descriptor sets! Pool may be out of memory or too fragmented.");
        }
        else {
            throw std::runtime_error("Failed to allocate descriptor sets! Vulkan error.");
        }
    }

    // 2. Update each Descriptor Set
    // Use vectors that grow, easier than fixed arrays if binding count varies slightly per set
    std::vector<VkWriteDescriptorSet> descriptorWrites;
    std::vector<VkDescriptorBufferInfo> tempBufferInfos; // Must persist until vkUpdateDescriptorSets
    std::vector<VkDescriptorImageInfo> tempImageInfos;   // Must persist until vkUpdateDescriptorSets

    for (uint32_t i = 0; i < setCount; ++i) {
        // Clear previous iteration's data (or reuse if possible and careful)
        descriptorWrites.clear();
        tempBufferInfos.clear();
        tempImageInfos.clear();

        // Get the specific bindings for this set index from the caller
		auto [bufferBindings, imageBindings] = getBindingsForSet(i);  // CRASHES HERE

        // Reserve space roughly (optional optimization)
        descriptorWrites.reserve(bufferBindings.size() + imageBindings.size());
        tempBufferInfos.reserve(bufferBindings.size());
        tempImageInfos.reserve(imageBindings.size());

        // Prepare writes for buffers
        for (const auto& bindingInfo : bufferBindings) {
            if (bindingInfo.buffer == VK_NULL_HANDLE) {
                fprintf(stderr, "Warning: Skipping null buffer for binding %u in descriptor set %u\n", bindingInfo.binding, i);
                continue; // Skip null resources
            }
            // Create the info struct and store it
            tempBufferInfos.push_back({
                .buffer = bindingInfo.buffer,
                .offset = bindingInfo.offset,
                .range = bindingInfo.range
                });

            // Create the write operation pointing to the *stored* info
            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = allocatedSets[i];
            write.dstBinding = bindingInfo.binding;
            write.dstArrayElement = 0; // Assuming not an array of UBOs here
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Or DYNAMIC etc.
            write.descriptorCount = 1;
            write.pBufferInfo = &tempBufferInfos.back(); // Point to the persistent info
            descriptorWrites.push_back(write);
        }

        // Prepare writes for images
        for (const auto& bindingInfo : imageBindings) {
            // Check for valid image view AND sampler for combined sampler type
            if (bindingInfo.imageView == VK_NULL_HANDLE || bindingInfo.sampler == VK_NULL_HANDLE) {
                fprintf(stderr, "Warning: Skipping null image view or sampler for binding %u, arrayElement %u in descriptor set %u\n", bindingInfo.binding, bindingInfo.arrayElement, i);
                continue; // Skip null resources
            }
            // Create the info struct and store it
            tempImageInfos.push_back({
                 .sampler = bindingInfo.sampler, // Order matters for {} initialization
                 .imageView = bindingInfo.imageView,
                 .imageLayout = bindingInfo.imageLayout
                });

            // Create the write operation pointing to the *stored* info
            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = allocatedSets[i];
            write.dstBinding = bindingInfo.binding;
            write.dstArrayElement = bindingInfo.arrayElement;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.descriptorCount = 1; // Assuming 1 descriptor per binding here
            write.pImageInfo = &tempImageInfos.back(); // Point to the persistent info
            descriptorWrites.push_back(write);
        }

        // Update the descriptor set for index 'i' if there's anything to write
        if (!descriptorWrites.empty()) {
            vkUpdateDescriptorSets(context->GetDevice(),
                static_cast<uint32_t>(descriptorWrites.size()),
                descriptorWrites.data(),
                0, nullptr);
        }
    }

    return allocatedSets; // Return the newly created and updated sets
}

void VulkanDescriptorManager::CleanupPool() {
    if (context && descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(context->GetDevice(), descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE; // Mark as destroyed
    }
}




