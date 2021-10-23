#include "environment_vulkan.hpp"
#include "utils_vulkan.hpp"

using namespace rb;

environment_vulkan::environment_vulkan(VkDevice device,
	VkQueue graphics_queue,
	VkCommandPool command_pool,
	VmaAllocator allocator,
    VkDescriptorSetLayout descriptor_set_layout,
	const environment_desc& desc)
	: environment(desc)
	, _device(device)
	, _allocator(allocator)
    , _descriptor_set_layout(descriptor_set_layout) {
	_create_image(desc);
	_update_image(graphics_queue, command_pool, desc);
	_create_image_view(desc);
	_create_sampler(desc);
    _create_irradiance_image();
    _create_prefilter_image();
    _create_descriptor_set();
}

environment_vulkan::~environment_vulkan() {
    vkDestroyDescriptorPool(_device, _descriptor_pool, nullptr);
    //vkDestroyDescriptorSetLayout(_device, _descriptor_set_layout, nullptr);

    vkDestroySampler(_device, _prefilter_sampler, nullptr);
    vkDestroyImageView(_device, _prefilter_image_view, nullptr);
    vmaDestroyImage(_allocator, _prefilter_image, _prefilter_allocation);

    vkDestroySampler(_device, _irradiance_sampler, nullptr);
    vkDestroyImageView(_device, _irradiance_image_view, nullptr);
    vmaDestroyImage(_allocator, _irradiance_image, _irradiance_allocation);

    vkDestroySampler(_device, _sampler, nullptr);
    vkDestroyImageView(_device, _image_view, nullptr);
    vmaDestroyImage(_allocator, _image, _allocation);
}

VkImage environment_vulkan::image() const {
    return _image;
}

VkImageView environment_vulkan::image_view() const {
    return _image_view;
}

VkSampler environment_vulkan::sampler() const {
    return _sampler;
}

VkImage environment_vulkan::irradiance_image() const {
    return _irradiance_image;
}

VkImageView environment_vulkan::irradiance_image_view() const {
    return _irradiance_image_view;
}

VkSampler environment_vulkan::irradiance_sampler() const {
    return _irradiance_sampler;
}

VkImage environment_vulkan::prefilter_image() const {
    return _prefilter_image;
}

VkImageView environment_vulkan::prefilter_image_view() const {
    return _prefilter_image_view;
}

VkSampler environment_vulkan::prefilter_sampler() const {
    return _prefilter_sampler;
}

VkDescriptorSet environment_vulkan::descriptor_set() const {
    return _descriptor_set;
}

void environment_vulkan::_create_image(const environment_desc& desc) {
    VkImageCreateInfo image_info;
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;
    image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_info.extent = { desc.size.x, desc.size.y, 1 };
    image_info.mipLevels = 1;
    image_info.arrayLayers = 6;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = 0;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocation_info = {};
    allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    RB_VK(vmaCreateImage(_allocator, &image_info, &allocation_info, &_image, &_allocation, nullptr),
        "Failed to create Vulkan image.");
}

void environment_vulkan::_update_image(VkQueue graphics_queue, VkCommandPool command_pool, const environment_desc& desc) {
    // Create staging buffer.
    VkBufferCreateInfo buffer_info;
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.pNext = nullptr;
    buffer_info.flags = 0;
    buffer_info.size = desc.size.x * desc.size.y * 4 * 6;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_info.queueFamilyIndexCount = 0;
    buffer_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo allocation_info = {};
    allocation_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;

    VkBuffer staging_buffer;
    VmaAllocation staging_buffer_allocation;

    RB_VK(vmaCreateBuffer(_allocator, &buffer_info, &allocation_info, &staging_buffer, &staging_buffer_allocation, nullptr),
        "Failed to create Vulkan buffer");

    // Transfer pixels into buffer.
    void* data;
    RB_VK(vmaMapMemory(_allocator, staging_buffer_allocation, &data), "Failed to map staging buffer memory");

    std::memcpy(data, desc.data, buffer_info.size);

    vmaUnmapMemory(_allocator, staging_buffer_allocation);

    auto command_buffer = utils_vulkan::begin_single_time_commands(_device, command_pool);

    for (std::size_t layer{ 0 }; layer < 6; ++layer) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = _image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = static_cast<std::uint32_t>(layer);
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkBufferImageCopy region{};
        region.bufferOffset = desc.size.x * desc.size.y * 4 * layer;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = static_cast<std::uint32_t>(layer);
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { desc.size.x, desc.size.y, 1 };

        vkCmdCopyBufferToImage(command_buffer, staging_buffer, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = _image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = static_cast<std::uint32_t>(layer);
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    utils_vulkan::end_single_time_commands(_device, graphics_queue, command_pool, command_buffer);

    vmaDestroyBuffer(_allocator, staging_buffer, staging_buffer_allocation);
}

void environment_vulkan::_create_image_view(const environment_desc& desc) {
    VkImageViewCreateInfo image_view_info;
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.pNext = nullptr;
    image_view_info.flags = 0;
    image_view_info.image = _image;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    image_view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 6;
    RB_VK(vkCreateImageView(_device, &image_view_info, nullptr, &_image_view),
        "Failed to create Vulkan image view");
}

void environment_vulkan::_create_sampler(const environment_desc& desc) {
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
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    RB_VK(vkCreateSampler(_device, &sampler_info, nullptr, &_sampler), "Failed to create Vulkan sampler");
}

void environment_vulkan::_create_irradiance_image() {
    VkImageCreateInfo image_info;
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;
    image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_info.extent = { 64, 64, 1 };
    image_info.mipLevels = 1;
    image_info.arrayLayers = 6;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
        | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = 0;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocation_info{};
    allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    RB_VK(vmaCreateImage(_allocator, &image_info, &allocation_info, &_irradiance_image, &_irradiance_allocation, nullptr),
        "Failed to create Vulkan image");

    VkImageViewCreateInfo image_view_info;
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.pNext = nullptr;
    image_view_info.flags = 0;
    image_view_info.image = _irradiance_image;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    image_view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 6;
    RB_VK(vkCreateImageView(_device, &image_view_info, nullptr, &_irradiance_image_view),
        "Failed to create Vulkan image view");

    VkSamplerCreateInfo sampler_info;
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.pNext = nullptr;
    sampler_info.flags = 0;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 0.0f;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    RB_VK(vkCreateSampler(_device, &sampler_info, nullptr, &_irradiance_sampler),
        "Failed to create Vulkan sampler");
}

void environment_vulkan::_create_prefilter_image() {
    VkImageCreateInfo image_info;
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;
    image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_info.extent = { 128, 128, 1 };
    image_info.mipLevels = 6;
    image_info.arrayLayers = 6;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
        | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = 0;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocation_info{};
    allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    RB_VK(vmaCreateImage(_allocator, &image_info, &allocation_info, &_prefilter_image, &_prefilter_allocation, nullptr),
        "Failed to create Vulkan image");

    VkImageViewCreateInfo image_view_info;
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.pNext = nullptr;
    image_view_info.flags = 0;
    image_view_info.image = _prefilter_image;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    image_view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 6;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 6;
    RB_VK(vkCreateImageView(_device, &image_view_info, nullptr, &_prefilter_image_view),
        "Failed to create Vulkan image view");

    VkSamplerCreateInfo sampler_info;
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.pNext = nullptr;
    sampler_info.flags = 0;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 0.0f;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    RB_VK(vkCreateSampler(_device, &sampler_info, nullptr, &_prefilter_sampler),
        "Failed to create Vulkan sampler");
}

void environment_vulkan::_create_descriptor_set() {
    VkDescriptorPoolSize pool_sizes[1]{
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 }
    };

    VkDescriptorPoolCreateInfo descriptor_pool_info;
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.pNext = nullptr;
    descriptor_pool_info.flags = 0;
    descriptor_pool_info.maxSets = 1;
    descriptor_pool_info.poolSizeCount = 1;
    descriptor_pool_info.pPoolSizes = pool_sizes;
    RB_VK(vkCreateDescriptorPool(_device, &descriptor_pool_info, nullptr, &_descriptor_pool),
        "Failed to create descriptor pool");

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = _descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &_descriptor_set_layout;
    RB_VK(vkAllocateDescriptorSets(_device, &descriptor_set_allocate_info, &_descriptor_set),
        "Failed to allocatore desctiptor set");

    VkDescriptorImageInfo image_infos[3]{
        { _sampler, _image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { _irradiance_sampler, _irradiance_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { _prefilter_sampler, _prefilter_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
    };

    VkWriteDescriptorSet write_infos[3]{
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _descriptor_set, 4, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[0], nullptr, nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _descriptor_set, 5, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[1], nullptr, nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _descriptor_set, 6, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[2], nullptr, nullptr },
    };

    vkUpdateDescriptorSets(_device, 3, write_infos, 0, nullptr);
}

