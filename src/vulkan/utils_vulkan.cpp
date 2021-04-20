#include "utils_vulkan.hpp"

#include <rabbit/range.hpp>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

using namespace rb;

VkBufferUsageFlags utils_vulkan::buffer_usage_flags(const buffer_type type) RB_NOEXCEPT {
    switch (type) {
        case buffer_type::vertex: return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        case buffer_type::index: return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        case buffer_type::uniform: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        default: RB_ASSERT(false, "Missing mapping"); return 0;
    }
}

std::uint32_t utils_vulkan::find_memory_type(VkPhysicalDevice physical_device, std::uint32_t type_filter, VkMemoryPropertyFlags properties) RB_NOEXCEPT {
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

    for (auto index : rb::make_range(0u, memory_properties.memoryTypeCount)) {
        if ((type_filter & (1 << index)) &&
            (memory_properties.memoryTypes[index].propertyFlags & properties) == properties) {
            return index;
        }
    }

    RB_ASSERT(false, "Failed to find suitable memory type");
    return 0;
}

VkFormat utils_vulkan::format(const vertex_format& format) RB_NOEXCEPT {
    switch (format.type) {
        case vertex_format_type::floating_point:
            switch (format.components) {
                case 1: return VK_FORMAT_R32_SFLOAT;
                case 2: return VK_FORMAT_R32G32_SFLOAT;
                case 3: return VK_FORMAT_R32G32B32_SFLOAT;
                case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
            }
            break;
        case vertex_format_type::integer:
            switch (format.components) {
                case 1: return VK_FORMAT_R32_UINT;
                case 2: return VK_FORMAT_R32G32_UINT;
                case 3: return VK_FORMAT_R32G32B32_UINT;
                case 4: return VK_FORMAT_R32G32B32A32_UINT;
            }
            break;
    }

    RB_ASSERT(false, "Failed to map Vulkan format");
    return VK_FORMAT_UNDEFINED;
}

VkFormat utils_vulkan::format(const texture_format& format) RB_NOEXCEPT {
    switch (format) {
        case texture_format::r8: return VK_FORMAT_R8_UNORM;
        case texture_format::rg8: return VK_FORMAT_R8G8_UNORM;
        case texture_format::rgba8: return VK_FORMAT_R8G8B8A8_UNORM;
        case texture_format::d24s8: return VK_FORMAT_D24_UNORM_S8_UINT;
    }

    RB_ASSERT(false, "Failed to map Vulkan format");
    return VK_FORMAT_UNDEFINED;
}

VkDescriptorType utils_vulkan::descriptor_type(const material_binding_type& type) RB_NOEXCEPT {
    switch (type) {
        case material_binding_type::texture:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case material_binding_type::uniform_buffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }

    RB_ASSERT(false, "Failed to map Vulkan descriptor type");
    return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

VkShaderStageFlags utils_vulkan::stage_flags(const std::uint32_t flags) RB_NOEXCEPT {
    VkShaderStageFlags bitfield = 0;

    if (flags & shader_stage_flags::vertex) {
        bitfield |= VK_SHADER_STAGE_VERTEX_BIT;
    }
    if (flags & shader_stage_flags::fragment) {
        bitfield |= VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    return bitfield;
}
