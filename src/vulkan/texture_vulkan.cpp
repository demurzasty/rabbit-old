#include "texture_vulkan.hpp"
#include "utils_vulkan.hpp"

using namespace rb;

texture_vulkan::texture_vulkan(VkDevice device, VkQueue graphics_queue, VkCommandPool command_pool, VmaAllocator allocator, const texture_desc& desc)
    : texture(desc)
    , _device(device)
    , _allocator(allocator) {
    _create_image(desc);

    if (desc.data) {
        _update_image(graphics_queue, command_pool, desc);
    }

    _create_image_view(desc);
    _create_sampler(desc);
}

texture_vulkan::~texture_vulkan() { 
    vkDestroySampler(_device, _sampler, nullptr);
    vkDestroyImageView(_device, _image_view, nullptr);
    vmaDestroyImage(_allocator, _image, _allocation);
}

VkImage texture_vulkan::image() const RB_NOEXCEPT {
    return _image;
}

VkImageView texture_vulkan::image_view() const RB_NOEXCEPT {
    return _image_view;
}

VkSampler texture_vulkan::sampler() const RB_NOEXCEPT {
    return _sampler;
}

void texture_vulkan::_create_image(const texture_desc& desc) {
    VkImageCreateInfo image_info;
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;
    image_info.flags = 0;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = utils_vulkan::format(desc.format);
    image_info.extent = { desc.size.x, desc.size.y, 1 };
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = 0;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    auto result2 = vkCreateImage(_device, &image_info, nullptr, &_image);
    RB_ASSERT(result2 == VK_SUCCESS, "Failed to create Vulkan image");

    VmaAllocationCreateInfo allocation_info = {};
	allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    RB_MAYBE_UNUSED auto result = vmaCreateImage(_allocator, &image_info, &allocation_info, &_image, &_allocation, nullptr);
    RB_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan image");
}

void texture_vulkan::_update_image(VkQueue graphics_queue, VkCommandPool command_pool, const texture_desc& desc) {
    // Create staging buffer.
    VkBufferCreateInfo buffer_info;
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.pNext = nullptr;
    buffer_info.flags = 0;
    buffer_info.size = desc.size.x * desc.size.y * bytes_per_pixel();
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_info.queueFamilyIndexCount = 0;
    buffer_info.pQueueFamilyIndices = nullptr;
    
    VmaAllocationCreateInfo allocation_info = {};
	allocation_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;

    VkBuffer staging_buffer;
    VmaAllocation staging_buffer_allocation;

    RB_MAYBE_UNUSED auto result = vmaCreateBuffer(_allocator, &buffer_info, &allocation_info, &staging_buffer, &staging_buffer_allocation, nullptr);
    RB_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan buffer");

    // Transfer pixels into buffer.
    void* data;
    result = vmaMapMemory(_allocator, staging_buffer_allocation, &data);
    RB_ASSERT(result == VK_SUCCESS, "Failed to map staging buffer memory");

    std::memcpy(data, desc.data, buffer_info.size);

    vmaUnmapMemory(_allocator, staging_buffer_allocation);

    _transition_image_layout(graphics_queue, command_pool, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    auto command_buffer = _begin_single_time_commands(command_pool);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { desc.size.x, desc.size.y, 1 };

    vkCmdCopyBufferToImage(command_buffer, staging_buffer, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    _end_single_time_commands(graphics_queue, command_pool, command_buffer);

    _transition_image_layout(graphics_queue, command_pool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vmaDestroyBuffer(_allocator, staging_buffer, staging_buffer_allocation);
}

void texture_vulkan::_create_image_view(const texture_desc& desc) {
    VkImageViewCreateInfo image_view_info;
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.pNext = nullptr;
    image_view_info.flags = 0;
    image_view_info.image = _image;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = utils_vulkan::format(desc.format);
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;

    RB_MAYBE_UNUSED auto result = vkCreateImageView(_device, &image_view_info, nullptr, &_image_view);
    RB_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan image view");
}

void texture_vulkan::_create_sampler(const texture_desc& desc) {
    // todo: vkGetPhysicalDeviceProperties for max anisotropy

    VkSamplerCreateInfo sampler_info;
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.pNext = nullptr;
    sampler_info.flags = 0;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 0.0f;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;

    RB_MAYBE_UNUSED auto result = vkCreateSampler(_device, &sampler_info, nullptr, &_sampler);
    RB_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan sampler");
}

void texture_vulkan::_transition_image_layout(VkQueue graphics_queue, VkCommandPool command_pool, VkImageLayout old_layout, VkImageLayout new_layout) {
    auto command_buffer = _begin_single_time_commands(command_pool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = _image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        RB_ASSERT(false, "Unsupported layout transition");
    }

    vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    _end_single_time_commands(graphics_queue, command_pool, command_buffer);
}

VkCommandBuffer texture_vulkan::_begin_single_time_commands(VkCommandPool command_pool) {
    // Create temporary buffer
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandPool = command_pool;
    allocate_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(_device, &allocate_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info;
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = nullptr;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = nullptr;
    
    // Begin registering commands
    vkBeginCommandBuffer(command_buffer, &begin_info);
    return command_buffer;
}

void texture_vulkan::_end_single_time_commands(VkQueue graphics_queue, VkCommandPool command_pool, VkCommandBuffer command_buffer) {
    // End registering commands
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(_device, command_pool, 1, &command_buffer);
}
