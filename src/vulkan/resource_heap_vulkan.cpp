#include "resource_heap_vulkan.hpp"
#include "utils_vulkan.hpp"
#include "material_vulkan.hpp"
#include "buffer_vulkan.hpp"

#include <rabbit/range.hpp>

using namespace rb;

resource_heap_vulkan::resource_heap_vulkan(VkDevice device, const resource_heap_desc& desc) 
    : resource_heap(desc)
    , _device(device) {
    _descriptor_sets.resize(desc.resource_views.size() / desc.material->bindings().size(), VK_NULL_HANDLE);

    _create_descriptor_pool(desc, material()->bindings());
    _create_descriptor_sets(desc);
    _update_descriptor_sets(desc, material()->bindings());
}

resource_heap_vulkan::~resource_heap_vulkan() {
    vkDestroyDescriptorPool(_device, _descriptor_pool, nullptr);
}
    
const std::vector<VkDescriptorSet>& resource_heap_vulkan::descriptor_sets() const RB_NOEXCEPT {
    return _descriptor_sets;
}

void resource_heap_vulkan::_create_descriptor_pool(const resource_heap_desc& desc, const std::vector<material_binding_desc>& bindings) {
    auto pool_sizes = std::make_unique<VkDescriptorPoolSize[]>(desc.resource_views.size());
    for (auto index : rb::make_range<std::size_t>(0u, desc.resource_views.size())) {
        pool_sizes[index].type = utils_vulkan::descriptor_type(bindings[index % desc.resource_views.size()].binding_type);
        pool_sizes[index].descriptorCount = 1;
    }

    // todo: compress pool sizes

    VkDescriptorPoolCreateInfo descriptor_pool_info;
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.pNext = nullptr;
    descriptor_pool_info.flags = 0;
    descriptor_pool_info.maxSets = static_cast<std::uint32_t>(_descriptor_sets.size());
    descriptor_pool_info.poolSizeCount = static_cast<std::uint32_t>(desc.resource_views.size());
    descriptor_pool_info.pPoolSizes = pool_sizes.get();

    RB_MAYBE_UNUSED auto result = vkCreateDescriptorPool(_device, &descriptor_pool_info, nullptr, &_descriptor_pool);
    RB_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan descriptol pool");
}

void resource_heap_vulkan::_create_descriptor_sets(const resource_heap_desc& desc) {
    auto native_material = std::static_pointer_cast<material_vulkan>(material());
    
    std::vector<VkDescriptorSetLayout> layouts(_descriptor_sets.size(), native_material->descriptor_set_layout());
    
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = _descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = static_cast<std::uint32_t>(_descriptor_sets.size());
    descriptor_set_allocate_info.pSetLayouts = layouts.data();

    RB_MAYBE_UNUSED auto result = vkAllocateDescriptorSets(_device, &descriptor_set_allocate_info, _descriptor_sets.data());
    RB_ASSERT(result == VK_SUCCESS, "Failed to allocate Vulkan descriptor sets");
}

void resource_heap_vulkan::_update_descriptor_sets(const resource_heap_desc& desc, const std::vector<material_binding_desc>& bindings) {
    auto container = std::make_unique<VkWriteDescriptorSet[]>(desc.resource_views.size());
    auto buffer_infos = std::make_unique<VkDescriptorBufferInfo[]>(desc.resource_views.size());

    for (auto index : rb::make_range<std::size_t>(0u, desc.resource_views.size())) {
        const auto& binding = bindings[index % bindings.size()];
        const auto& resource_view = desc.resource_views[index];

        container[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        container[index].pNext = nullptr;
        container[index].dstBinding = binding.slot;
        container[index].dstArrayElement = 0;
        container[index].descriptorCount = 1;
        container[index].descriptorType = utils_vulkan::descriptor_type(binding.binding_type);
        container[index].pBufferInfo = nullptr;
        container[index].pImageInfo = nullptr;
        container[index].pTexelBufferView = nullptr;
        container[index].dstSet = _descriptor_sets[index / bindings.size()];

        switch (binding.binding_type) {
            case material_binding_type::uniform_buffer: {
                buffer_infos[index].buffer = std::static_pointer_cast<buffer_vulkan>(resource_view.buffer)->buffer();
                buffer_infos[index].offset = 0;
                buffer_infos[index].range = resource_view.buffer->size();
                
                container[index].descriptorType = utils_vulkan::descriptor_type(binding.binding_type);
                container[index].pBufferInfo = &buffer_infos[index];
                break;
            }
            case material_binding_type::sampler:
                
                break;
            case material_binding_type::texture:
                
                break;
        }
    }

    vkUpdateDescriptorSets(_device, static_cast<std::uint32_t>(desc.resource_views.size()), container.get(), 0, nullptr);
}
