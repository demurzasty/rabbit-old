#include "buffer_vulkan.hpp"

#include <rabbit/core/exception.hpp>

#include <map>

using namespace rb;

namespace rb {
    std::map<buffer_type, VkBufferUsageFlags> usages = {
        { buffer_type::vertex, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT },
        { buffer_type::index, VK_BUFFER_USAGE_INDEX_BUFFER_BIT },
        { buffer_type::uniform, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT }
    };
}

buffer_vulkan::buffer_vulkan(VkPhysicalDevice physical_device, VkDevice device, const buffer_desc& desc)
    : rb::buffer(desc)
    , _physical_device(physical_device)
    , _device(device) {
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = desc.size;
    buffer_info.usage = usages.at(desc.type);
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(_device, &buffer_info, nullptr, &_buffer) != VK_SUCCESS) {
        throw make_exception("Failed to create vertex buffer!");
    }

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(_device, _buffer, &memory_requirements);

    VkMemoryAllocateInfo memory_allocate_info{};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = find_memory_type(_physical_device, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(_device, &memory_allocate_info, nullptr, &_memory) != VK_SUCCESS) {
        throw make_exception("Failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(_device, _buffer, _memory, 0);

    if (desc.data) {
        void* data;
        vkMapMemory(_device, _memory, 0, buffer_info.size, 0, &data);
        std::memcpy(data, desc.data, desc.size);
        vkUnmapMemory(_device, _memory);
    }
}

buffer_vulkan::~buffer_vulkan() {
}

void* buffer_vulkan::map() {
    void* ptr;
    vkMapMemory(_device, _memory, 0, size(), 0, &ptr);
    return ptr;
}

void buffer_vulkan::unmap() {
    vkUnmapMemory(_device, _memory);
}

VkBuffer buffer_vulkan::buffer() const {
    return _buffer;
}

VkDeviceMemory buffer_vulkan::memory() const {
    return _memory;
}

void buffer_vulkan::update(const void* data, std::size_t size) {
    void* ptr;
    vkMapMemory(_device, _memory, 0, size, 0, &ptr);
    std::memcpy(ptr, data, size);
    vkUnmapMemory(_device, _memory);
}

std::uint32_t buffer_vulkan::find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(_physical_device, &memory_properties);

    for (std::uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw make_exception("Failed to find suitable memory type!");
}
