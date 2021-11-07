#include "mesh_vulkan.hpp"
#include "utils_vulkan.hpp"

using namespace rb;

mesh_vulkan::mesh_vulkan(VkDevice device, VmaAllocator allocator, const mesh_desc& desc)
	: mesh(desc)
	, _device(device)
	, _allocator(allocator) {
    VkBufferCreateInfo vertex_buffer_info;
    vertex_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertex_buffer_info.pNext = nullptr;
    vertex_buffer_info.flags = 0;
    vertex_buffer_info.size = desc.vertices.size_bytes();
    vertex_buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vertex_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vertex_buffer_info.queueFamilyIndexCount = 0;
    vertex_buffer_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo vertex_allocation_info{};
    vertex_allocation_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    RB_VK(vmaCreateBuffer(allocator, &vertex_buffer_info, &vertex_allocation_info, &_vertex_buffer, &_vertex_allocation, nullptr),
        "Failed to create Vulkan buffer.");

    void* ptr;
    RB_VK(vmaMapMemory(_allocator, _vertex_allocation, &ptr), "Failed to map memory");
    std::memcpy(ptr, desc.vertices.data(), desc.vertices.size_bytes());
    vmaUnmapMemory(_allocator, _vertex_allocation);

    VkBufferCreateInfo index_buffer_info;
    index_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    index_buffer_info.pNext = nullptr;
    index_buffer_info.flags = 0;
    index_buffer_info.size = desc.indices.size_bytes();
    index_buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    index_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    index_buffer_info.queueFamilyIndexCount = 0;
    index_buffer_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo index_allocation_info{};
    index_allocation_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    RB_VK(vmaCreateBuffer(allocator, &index_buffer_info, &index_allocation_info, &_index_buffer, &_index_allocation, nullptr),
        "Failed to create Vulkan buffer.");

    RB_VK(vmaMapMemory(_allocator, _index_allocation, &ptr), "Failed to map memory");
    std::memcpy(ptr, desc.indices.data(), desc.indices.size_bytes());
    vmaUnmapMemory(_allocator, _index_allocation);
}

mesh_vulkan::~mesh_vulkan() {
    vmaDestroyBuffer(_allocator, _vertex_buffer, _vertex_allocation);
    vmaDestroyBuffer(_allocator, _index_buffer, _index_allocation);
}

VkBuffer mesh_vulkan::vertex_buffer() const {
    return _vertex_buffer;
}

VkBuffer mesh_vulkan::index_buffer() const {
    return _index_buffer;
}
