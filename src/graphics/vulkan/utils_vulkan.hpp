#pragma once

#include <rabbit/core/config.hpp>
#include <rabbit/graphics/buffer.hpp>
#include <rabbit/graphics/vertex.hpp>
#include <rabbit/graphics/material.hpp>
#include <rabbit/graphics/texture.hpp>
#include <rabbit/graphics/shader.hpp>

#include <volk.h>

#include <cstdint>

namespace rb {
    struct utils_vulkan {
        RB_NODISCARD static VkBufferUsageFlags buffer_usage_flags(const buffer_type type) RB_NOEXCEPT;

        RB_NODISCARD static std::uint32_t find_memory_type(VkPhysicalDevice physical_device, std::uint32_t type_filter, VkMemoryPropertyFlags properties) RB_NOEXCEPT;

        RB_NODISCARD static VkFormat format(const vertex_format& format) RB_NOEXCEPT;

        RB_NODISCARD static VkFormat format(const texture_format& format) RB_NOEXCEPT;

        RB_NODISCARD static VkDescriptorType descriptor_type(const shader_binding_type& type) RB_NOEXCEPT;

        RB_NODISCARD static VkShaderStageFlags stage_flags(const std::uint32_t flags) RB_NOEXCEPT;
    };
}
