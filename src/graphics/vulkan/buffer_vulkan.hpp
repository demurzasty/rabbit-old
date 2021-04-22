#pragma once

#include <rabbit/core/config.hpp>
#include <rabbit/graphics/buffer.hpp>

#include <volk.h>
#include <vk_mem_alloc.h>

namespace rb {
    class buffer_vulkan : public buffer {
    public:
        buffer_vulkan(VkPhysicalDevice physical_device, VkDevice device, VmaAllocator allocator, const buffer_desc& desc);

        ~buffer_vulkan();

        void update(const void* data) override;

        RB_NODISCARD VkBuffer buffer() const RB_NOEXCEPT;

    private:
        VkDevice _device;
        VmaAllocator _allocator;
        VkBuffer _buffer;
        VmaAllocation _allocation;
        //VkDeviceMemory _memory;
    };
}