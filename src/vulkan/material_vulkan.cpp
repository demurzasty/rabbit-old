#include "material_vulkan.hpp"
#include "texture_vulkan.hpp"
#include "utils_vulkan.hpp"

using namespace rb;

material_vulkan::material_vulkan(VkDevice device,
    VmaAllocator allocator,
    VkDescriptorSetLayout descriptor_set_layout,
    const material_desc& desc)
    : material(desc)
    , _device(device)
    , _allocator(allocator)
    , _descriptor_set_layout(descriptor_set_layout) {
    VkBufferCreateInfo uniform_buffer_info;
    uniform_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    uniform_buffer_info.pNext = nullptr;
    uniform_buffer_info.flags = 0;
    uniform_buffer_info.size = sizeof(data);
    uniform_buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    uniform_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    uniform_buffer_info.queueFamilyIndexCount = 0;
    uniform_buffer_info.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo uniform_buffer_allocation_info;
    uniform_buffer_allocation_info.flags = 0;
    uniform_buffer_allocation_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    uniform_buffer_allocation_info.requiredFlags = 0;
    uniform_buffer_allocation_info.preferredFlags = 0;
    uniform_buffer_allocation_info.memoryTypeBits = 0;
    uniform_buffer_allocation_info.pool = VK_NULL_HANDLE;
    uniform_buffer_allocation_info.pUserData = nullptr;

    RB_VK(vmaCreateBuffer(_allocator,
        &uniform_buffer_info,
        &uniform_buffer_allocation_info,
        &_uniform_buffer,
        &_uniform_buffer_allocation,
        nullptr), "Failed to create Vulkan uniform buffer");

    data data;
    data.base_color = desc.base_color;
    data.roughness = desc.roughness;
    data.metallic = desc.metallic;

    void* ptr;
    RB_VK(vmaMapMemory(_allocator, _uniform_buffer_allocation, &ptr), "Failed to map memory");
    std::memcpy(ptr, &data, sizeof(data));
    vmaUnmapMemory(_allocator, _uniform_buffer_allocation);

    VkDescriptorPoolSize pool_sizes[2]{
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5 }
    };

    VkDescriptorPoolCreateInfo descriptor_pool_info;
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.pNext = nullptr;
    descriptor_pool_info.flags = 0;
    descriptor_pool_info.maxSets = 1;
    descriptor_pool_info.poolSizeCount = 2;
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

    VkDescriptorBufferInfo buffer_infos[1]{
        { _uniform_buffer, 0, sizeof(data) }
    };

    const auto image_view = [](const std::shared_ptr<texture>& texture) {
        return std::static_pointer_cast<texture_vulkan>(texture)->image_view();
    };

    const auto sampler = [](const std::shared_ptr<texture>& texture) {
        return std::static_pointer_cast<texture_vulkan>(texture)->sampler();
    };

    VkDescriptorImageInfo image_infos[5]{
        { sampler(desc.albedo_map), image_view(desc.albedo_map), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { sampler(desc.normal_map), image_view(desc.normal_map), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { sampler(desc.roughness_map), image_view(desc.roughness_map), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { sampler(desc.metallic_map), image_view(desc.metallic_map), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        { sampler(desc.emissive_map), image_view(desc.emissive_map), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
    };

    VkWriteDescriptorSet write_infos[6]{
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _descriptor_set, 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &buffer_infos[0], nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _descriptor_set, 1, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[0], nullptr, nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _descriptor_set, 2, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[1], nullptr, nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _descriptor_set, 3, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[2], nullptr, nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _descriptor_set, 4, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[3], nullptr, nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _descriptor_set, 5, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &image_infos[4], nullptr, nullptr },
    };

    vkUpdateDescriptorSets(_device, 6, write_infos, 0, nullptr);
}

material_vulkan::~material_vulkan() {
    vkDestroyDescriptorPool(_device, _descriptor_pool, nullptr);
    vmaDestroyBuffer(_allocator, _uniform_buffer, _uniform_buffer_allocation);
}

VkDescriptorSet material_vulkan::descriptor_set() const {
    return _descriptor_set;
}
