#include "material_vulkan.hpp"
#include "texture_vulkan.hpp"
#include "utils_vulkan.hpp"

using namespace rb;

material_vulkan::material_vulkan(VkDevice device,
    VmaAllocator allocator,
    const material_desc& desc)
    : material(desc)
    , _device(device)
    , _allocator(allocator) {
    std::vector<VkDescriptorSetLayoutBinding> material_bindings;
    material_bindings.push_back({ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr });

    std::uint32_t map_count{ 0 };
    if (flags() & material_flags::albedo_map_bit) {
        map_count++;
    }

    if (flags() & material_flags::normal_map_bit) {
        map_count++;
    }

    if (flags() & material_flags::roughness_map_bit) {
        map_count++;
    }

    if (flags() & material_flags::metallic_map_bit) {
        map_count++;
    }

    if (flags() & material_flags::emissive_map_bit) {
        map_count++;
    }

    if (flags() & material_flags::ambient_map_bit) {
        map_count++;
    }
    
    if (map_count > 0) {
        material_bindings.push_back({ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, map_count, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr });
    }

    VkDescriptorSetLayoutCreateInfo material_descriptor_set_layout_info;
    material_descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    material_descriptor_set_layout_info.pNext = nullptr;
    material_descriptor_set_layout_info.flags = 0;
    material_descriptor_set_layout_info.bindingCount = static_cast<std::uint32_t>(material_bindings.size());
    material_descriptor_set_layout_info.pBindings = material_bindings.data();
    RB_VK(vkCreateDescriptorSetLayout(_device, &material_descriptor_set_layout_info, nullptr, &_descriptor_set_layout),
        "Failed to create Vulkan descriptor set layout");

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
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6 }
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
        return texture ? std::static_pointer_cast<texture_vulkan>(texture)->image_view() : VK_NULL_HANDLE;
    };

    const auto sampler = [](const std::shared_ptr<texture>& texture) {
        return texture ? std::static_pointer_cast<texture_vulkan>(texture)->sampler() : VK_NULL_HANDLE;
    };

    std::vector<VkDescriptorImageInfo> image_infos;

    if (flags() & material_flags::albedo_map_bit) {
        image_infos.push_back({ sampler(desc.albedo_map), image_view(desc.albedo_map), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
    }

    if (flags() & material_flags::normal_map_bit) {
        image_infos.push_back({ sampler(desc.normal_map), image_view(desc.normal_map), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
    }

    if (flags() & material_flags::roughness_map_bit) {
        image_infos.push_back({ sampler(desc.roughness_map), image_view(desc.roughness_map), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
    }

    if (flags() & material_flags::metallic_map_bit) {
        image_infos.push_back({ sampler(desc.metallic_map), image_view(desc.metallic_map), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
    }

    if (flags() & material_flags::emissive_map_bit) {
        image_infos.push_back({ sampler(desc.emissive_map), image_view(desc.emissive_map), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
    }

    if (flags() & material_flags::ambient_map_bit) {
        image_infos.push_back({ sampler(desc.ambient_map), image_view(desc.ambient_map), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
    }

    std::vector<VkWriteDescriptorSet> write_infos;
    write_infos.push_back({ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _descriptor_set, 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &buffer_infos[0], nullptr });
    
    if (!image_infos.empty()) {
        write_infos.push_back({ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, _descriptor_set, 1, 0, static_cast<std::uint32_t>(image_infos.size()), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, image_infos.data(), nullptr, nullptr });
    }

    vkUpdateDescriptorSets(_device, static_cast<std::uint32_t>(write_infos.size()), write_infos.data(), 0, nullptr);
}

material_vulkan::~material_vulkan() {
    vkDestroyDescriptorSetLayout(_device, _descriptor_set_layout, nullptr);
    vkDestroyDescriptorPool(_device, _descriptor_pool, nullptr);
    vmaDestroyBuffer(_allocator, _uniform_buffer, _uniform_buffer_allocation);
}

VkDescriptorSetLayout material_vulkan::descriptor_set_layout() const {
    return _descriptor_set_layout;
}

VkDescriptorSet material_vulkan::descriptor_set() const {
    return _descriptor_set;
}
