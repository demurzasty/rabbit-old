#pragma once 

#include <rabbit/graphics/buffer.hpp>

#include <volk.h>

namespace rb {
    class buffer_vulkan : public buffer {
    public:
        buffer_vulkan(VkPhysicalDevice physical_device, VkDevice device, const buffer_desc& desc);

        ~buffer_vulkan();

        void* map() override;

        void unmap() override;

        VkBuffer buffer() const;

        VkDeviceMemory memory() const;
        
    protected:
        void update(const void* data, std::size_t size) override;
    
    private:
        std::uint32_t find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties) const;

    private:
        VkPhysicalDevice _physical_device;
        VkDevice _device;
        VkBuffer _buffer;
        VkDeviceMemory _memory;
    };
}