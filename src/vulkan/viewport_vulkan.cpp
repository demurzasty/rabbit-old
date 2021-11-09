#include "viewport_vulkan.hpp"
#include "utils_vulkan.hpp"

#include <rabbit/graphics.hpp>

using namespace rb;

viewport_vulkan::viewport_vulkan(VkDevice device,
    VmaAllocator allocator,
    VkFormat depth_format,
    VkRenderPass depth_render_pass,
    VkDescriptorSetLayout depth_descriptor_set_layout,
    VkRenderPass forward_render_pass,
    VkDescriptorSetLayout forward_descriptor_set_layout,
    VkRenderPass postprocess_render_pass,
    VkDescriptorSetLayout postprocess_descriptor_set_layout,
    VkRenderPass fill_render_pass,
    VkDescriptorSetLayout fill_descriptor_set_layout,
    const viewport_desc& desc)
    : viewport(desc)
    , _device(device)
    , _allocator(allocator)
    , _depth_render_pass(depth_render_pass)
    , _depth_descriptor_set_layout(depth_descriptor_set_layout)
    , _forward_render_pass(forward_render_pass)
    , _forward_descriptor_set_layout(forward_descriptor_set_layout)
    , _postprocess_render_pass(postprocess_render_pass)
    , _postprocess_descriptor_set_layout(postprocess_descriptor_set_layout) {
    _create_descriptor_pool(desc);
    _create_sampler(desc);
    _create_depth(desc, depth_format);
    _create_light(desc);
    _create_forward(desc);
    _create_postprocess(desc);
    _create_fill(fill_render_pass, fill_descriptor_set_layout, desc);
}

viewport_vulkan::~viewport_vulkan() {
    vkDestroyFramebuffer(_device, _fill_framebuffer, nullptr);
    vkDestroyImageView(_device, _fill_image_view, nullptr);
    vmaDestroyImage(_allocator, _fill_image, _fill_image_allocation);

    vkDestroyFramebuffer(_device, _postprocess_framebuffer, nullptr);
    vkDestroyImageView(_device, _postprocess_image_view, nullptr);
    vmaDestroyImage(_allocator, _postprocess_image, _postprocess_image_allocation);

    vkDestroyFramebuffer(_device, _forward_framebuffer, nullptr);
    vkDestroyImageView(_device, _forward_image_view, nullptr);
    vmaDestroyImage(_allocator, _forward_image, _forward_image_allocation);

    vmaDestroyBuffer(_allocator, _visible_light_indices_buffer, _visible_light_indices_buffer_allocation);
    vmaDestroyBuffer(_allocator, _light_buffer, _light_buffer_allocation);

    vkDestroyFramebuffer(_device, _depth_framebuffer, nullptr);
    vkDestroyImageView(_device, _depth_image_view, nullptr);
    vmaDestroyImage(_allocator, _depth_image, _depth_image_allocation);

    vkDestroySampler(_device, _sampler, nullptr);

    vkDestroyDescriptorPool(_device, _descriptor_pool, nullptr);
}

void viewport_vulkan::begin_depth_pass(VkCommandBuffer command_buffer) {
    VkClearValue clear_values[1];
    clear_values[0].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo render_pass_begin_info;
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = _depth_render_pass;
    render_pass_begin_info.framebuffer = _depth_framebuffer;
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = { size().x, size().y };
    render_pass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(*clear_values);
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{
        0.0f, 0.0f,
        static_cast<float>(size().x), static_cast<float>(size().y),
        0.0f, 1.0f
    };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{
        { 0, 0 },
        { size().x, size().y }
    };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

void viewport_vulkan::end_depth_pass(VkCommandBuffer command_buffer) {
    vkCmdEndRenderPass(command_buffer);
}

void viewport_vulkan::begin_forward_pass(VkCommandBuffer command_buffer) {
    // Do not clear depth, copy from _depth_image
    VkClearValue clear_values[1];
    clear_values[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };

    VkRenderPassBeginInfo render_pass_begin_info;
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = _forward_render_pass;
    render_pass_begin_info.framebuffer = _forward_framebuffer;
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = { size().x, size().y };
    render_pass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(*clear_values);
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{
        0.0f, 0.0f,
        static_cast<float>(size().x), static_cast<float>(size().y),
        0.0f, 1.0f
    };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{
        { 0, 0 },
        { size().x, size().y }
    };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

void viewport_vulkan::end_forward_pass(VkCommandBuffer command_buffer) {
    vkCmdEndRenderPass(command_buffer);
}

void viewport_vulkan::begin_postprocess_pass(VkCommandBuffer command_buffer) {
    VkClearValue clear_values[1];
    clear_values[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };

    VkRenderPassBeginInfo render_pass_begin_info;
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = _postprocess_render_pass;
    render_pass_begin_info.framebuffer = _postprocess_framebuffer;
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = { size().x, size().y };
    render_pass_begin_info.clearValueCount = sizeof(clear_values) / sizeof(*clear_values);
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{
        0.0f, 0.0f,
        static_cast<float>(size().x), static_cast<float>(size().y),
        0.0f, 1.0f
    };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{
        { 0, 0 },
        { size().x, size().y }
    };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

void viewport_vulkan::end_postprocess_pass(VkCommandBuffer command_buffer) {
    vkCmdEndRenderPass(command_buffer);
}

VkDescriptorSet viewport_vulkan::depth_descriptor_set() const {
    return _depth_descriptor_set;
}

VkDescriptorSet viewport_vulkan::forward_descriptor_set() const {
    return _forward_descriptor_set;
}

VkDescriptorSet viewport_vulkan::postprocess_descriptor_set() const {
    return _postprocess_descriptor_set;
}

VkDescriptorSet viewport_vulkan::fill_descriptor_set() const {
    return _fill_descriptor_set;
}

VkFramebuffer viewport_vulkan::fill_framebuffer() const {
    return _fill_framebuffer;
}

void viewport_vulkan::_create_descriptor_pool(const viewport_desc& desc) {
    VkDescriptorPoolSize pool_sizes[1]{
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 9 }
    };

    VkDescriptorPoolCreateInfo descriptor_pool_info;
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.pNext = nullptr;
    descriptor_pool_info.flags = 0;
    descriptor_pool_info.maxSets = 6;
    descriptor_pool_info.poolSizeCount = 1;
    descriptor_pool_info.pPoolSizes = pool_sizes;
    RB_VK(vkCreateDescriptorPool(_device, &descriptor_pool_info, nullptr, &_descriptor_pool),
        "Failed to create descriptor pool");
}

void viewport_vulkan::_create_sampler(const viewport_desc& desc) {
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
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    RB_VK(vkCreateSampler(_device, &sampler_info, nullptr, &_sampler), "Failed to create Vulkan sampler");
}

void viewport_vulkan::_create_depth(const viewport_desc& desc, VkFormat depth_format) {
    VkImageCreateInfo image_info;
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;
    image_info.flags = 0;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = depth_format;
    image_info.extent = { desc.size.x, desc.size.y, 1 };
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = 0;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocation_info{};
    allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    RB_VK(vmaCreateImage(_allocator, &image_info, &allocation_info, &_depth_image, &_depth_image_allocation, nullptr),
        "Failed to create Vulkan image");

    VkImageViewCreateInfo image_view_info;
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.pNext = nullptr;
    image_view_info.flags = 0;
    image_view_info.image = _depth_image;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = depth_format;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;
    RB_VK(vkCreateImageView(_device, &image_view_info, nullptr, &_depth_image_view), "Failed to create image view");

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = _descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &_depth_descriptor_set_layout;
    RB_VK(vkAllocateDescriptorSets(_device, &descriptor_set_allocate_info, &_depth_descriptor_set),
        "Failed to allocatore desctiptor set");

    VkFramebufferCreateInfo framebuffer_info;
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.pNext = nullptr;
    framebuffer_info.flags = 0;
    framebuffer_info.renderPass = _depth_render_pass;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = &_depth_image_view;
    framebuffer_info.width = desc.size.x;
    framebuffer_info.height = desc.size.y;
    framebuffer_info.layers = 1;
    RB_VK(vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &_depth_framebuffer),
        "Failed to create Vulkan framebuffer");

    VkDescriptorImageInfo image_infos[1]{
        { _sampler, _depth_image_view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL }
    };

    VkWriteDescriptorSet write_infos[1]{
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _depth_descriptor_set, 0, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[0], nullptr, nullptr },
    };

    vkUpdateDescriptorSets(_device, 1, write_infos, 0, nullptr);
}

void viewport_vulkan::_create_light(const viewport_desc& desc) {
    VkBufferCreateInfo light_buffer_info;
    light_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    light_buffer_info.pNext = nullptr;
    light_buffer_info.flags = 0;
    light_buffer_info.size = graphics_limits::max_lights * sizeof(light_data);
    light_buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    light_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    light_buffer_info.queueFamilyIndexCount = 0;
    light_buffer_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo light_allocation_info{};
    light_allocation_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    RB_VK(vmaCreateBuffer(_allocator, &light_buffer_info, &light_allocation_info, &_light_buffer, &_light_buffer_allocation, nullptr),
        "Failed to create Vulkan buffer.");

    const auto work_groups_x = (size().x + (size().x % 16)) / 16;
    const auto work_groups_y = (size().y + (size().y % 16)) / 16;
    const auto number_of_tiles = work_groups_x * work_groups_y;

    VkBufferCreateInfo visible_light_indices_buffer_info;
    visible_light_indices_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    visible_light_indices_buffer_info.pNext = nullptr;
    visible_light_indices_buffer_info.flags = 0;
    visible_light_indices_buffer_info.size = number_of_tiles * sizeof(visible_index) * graphics_limits::max_lights;
    visible_light_indices_buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    visible_light_indices_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    visible_light_indices_buffer_info.queueFamilyIndexCount = 0;
    visible_light_indices_buffer_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo visible_light_indices_allocation_info{};
    visible_light_indices_allocation_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    RB_VK(vmaCreateBuffer(_allocator, &visible_light_indices_buffer_info, &visible_light_indices_allocation_info, &_visible_light_indices_buffer, &_visible_light_indices_buffer_allocation, nullptr),
        "Failed to create Vulkan buffer.");
}

void viewport_vulkan::_create_forward(const viewport_desc& desc) {
    VkImageCreateInfo image_info;
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;
    image_info.flags = 0;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_info.extent = { desc.size.x, desc.size.y, 1 };
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
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
    RB_VK(vmaCreateImage(_allocator, &image_info, &allocation_info, &_forward_image, &_forward_image_allocation, nullptr),
        "Failed to create Vulkan image");

    VkImageViewCreateInfo image_view_info;
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.pNext = nullptr;
    image_view_info.flags = 0;
    image_view_info.image = _forward_image;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;
    RB_VK(vkCreateImageView(_device, &image_view_info, nullptr, &_forward_image_view), "Failed to create image view");

    VkImageView framebuffer_images_views[2]{ _forward_image_view, _depth_image_view };

    VkFramebufferCreateInfo framebuffer_info;
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.pNext = nullptr;
    framebuffer_info.flags = 0;
    framebuffer_info.renderPass = _forward_render_pass;
    framebuffer_info.attachmentCount = 2;
    framebuffer_info.pAttachments = framebuffer_images_views;
    framebuffer_info.width = desc.size.x;
    framebuffer_info.height = desc.size.y;
    framebuffer_info.layers = 1;
    RB_VK(vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &_forward_framebuffer),
        "Failed to create Vulkan framebuffer");

    VkDescriptorSetLayoutBinding bindings[1]{
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = _descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &_forward_descriptor_set_layout;
    RB_VK(vkAllocateDescriptorSets(_device, &descriptor_set_allocate_info, &_forward_descriptor_set),
        "Failed to allocatore desctiptor set");

    VkDescriptorImageInfo image_infos[1]{
        { _sampler, _forward_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
    };

    VkWriteDescriptorSet write_infos[1]{
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _forward_descriptor_set, 0, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[0], nullptr, nullptr },
    };

    vkUpdateDescriptorSets(_device, 1, write_infos, 0, nullptr);
}

void viewport_vulkan::_create_postprocess(const viewport_desc& desc) {
    VkImageCreateInfo image_info;
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;
    image_info.flags = 0;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_info.extent = { desc.size.x, desc.size.y, 1 };
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
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
    RB_VK(vmaCreateImage(_allocator, &image_info, &allocation_info, &_postprocess_image, &_postprocess_image_allocation, nullptr),
        "Failed to create Vulkan image");

    VkImageViewCreateInfo image_view_info;
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.pNext = nullptr;
    image_view_info.flags = 0;
    image_view_info.image = _postprocess_image;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;
    RB_VK(vkCreateImageView(_device, &image_view_info, nullptr, &_postprocess_image_view), "Failed to create image view");

    VkFramebufferCreateInfo framebuffer_info;
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.pNext = nullptr;
    framebuffer_info.flags = 0;
    framebuffer_info.renderPass = _postprocess_render_pass;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = &_postprocess_image_view;
    framebuffer_info.width = desc.size.x;
    framebuffer_info.height = desc.size.y;
    framebuffer_info.layers = 1;
    RB_VK(vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &_postprocess_framebuffer),
        "Failed to create Vulkan framebuffer");

    VkDescriptorSetLayoutBinding bindings[1]{
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    VkDescriptorSetLayout layouts[]{
        _postprocess_descriptor_set_layout
    };

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = _descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = layouts;
    RB_VK(vkAllocateDescriptorSets(_device, &descriptor_set_allocate_info, &_postprocess_descriptor_set),
        "Failed to allocatore desctiptor set");

    VkDescriptorImageInfo image_infos[1]{
        { _sampler, _postprocess_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
    };

    VkWriteDescriptorSet write_infos[1]{
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _postprocess_descriptor_set, 0, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[0], nullptr, nullptr },
    };

    vkUpdateDescriptorSets(_device, 1, write_infos, 0, nullptr);
}

void viewport_vulkan::_create_fill(VkRenderPass fill_render_pass, VkDescriptorSetLayout fill_descriptor_set_layout, const viewport_desc& desc) {
    VkImageCreateInfo image_info;
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;
    image_info.flags = 0;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = VK_FORMAT_R8_UNORM;
    image_info.extent = { size().x, size().y, 1 };
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = 0;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocation_info = {};
    allocation_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    RB_VK(vmaCreateImage(_allocator, &image_info, &allocation_info, &_fill_image, &_fill_image_allocation, nullptr),
        "Failed to create Vulkan image.");

    VkImageViewCreateInfo image_view_info;
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.pNext = nullptr;
    image_view_info.flags = 0;
    image_view_info.image = _fill_image;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = VK_FORMAT_R8_UNORM;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_R;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_G;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_B;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_A;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;
    RB_VK(vkCreateImageView(_device, &image_view_info, nullptr, &_fill_image_view),
        "Failed to create Vulkan image view");

    VkFramebufferCreateInfo framebuffer_info;
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.pNext = nullptr;
    framebuffer_info.flags = 0;
    framebuffer_info.renderPass = fill_render_pass;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = &_fill_image_view;
    framebuffer_info.width = size().x;
    framebuffer_info.height = size().y;
    framebuffer_info.layers = 1;
    RB_VK(vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &_fill_framebuffer),
        "Failed to create fill framebuffer");

    VkDescriptorSetLayoutBinding bindings[1]{
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = _descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &fill_descriptor_set_layout;
    RB_VK(vkAllocateDescriptorSets(_device, &descriptor_set_allocate_info, &_fill_descriptor_set),
        "Failed to allocatore desctiptor set");

    VkDescriptorImageInfo image_infos[1]{
        { _sampler, _fill_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
    };

    VkWriteDescriptorSet write_infos[1]{
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _fill_descriptor_set, 0, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[0], nullptr, nullptr },
    };

    vkUpdateDescriptorSets(_device, 1, write_infos, 0, nullptr);
}