#include "shader_data_vulkan.hpp"
#include "buffer_vulkan.hpp"
#include "shader_vulkan.hpp"
#include "texture_vulkan.hpp"
#include "utils_vulkan.hpp"

#include <rabbit/core/range.hpp>

#include <memory>

using namespace rb;

shader_data_vulkan::shader_data_vulkan(VkDevice device, const shader_data_desc& desc)
    : shader_data(desc)
    , _device(device) {
    _descriptor_sets.resize(shader()->bindings().size(), VK_NULL_HANDLE);

    _create_descriptor_pool(desc, shader()->bindings());
    _create_descriptor_sets(desc);

    _update_descriptor_sets(desc, shader()->bindings());
}

shader_data_vulkan::~shader_data_vulkan() {
    vkDestroyDescriptorPool(_device, _descriptor_pool, nullptr);
}

void shader_data_vulkan::bind_resource(std::size_t slot, const std::shared_ptr<buffer>& buffer) {
    RB_ASSERT(buffer, "Buffer is not provided");
    RB_ASSERT(buffer->type() == buffer_type::uniform, "Buffer must be uniform type");

    RB_MAYBE_UNUSED const auto binding = shader()->binding(slot);
    RB_ASSERT(binding.has_value(), "No binding with provided slot index");
    RB_ASSERT(binding.value().binding_type == shader_binding_type::uniform_buffer, "Binding is not uniform type");

    VkDescriptorBufferInfo buffer_info;
    buffer_info.buffer = std::static_pointer_cast<buffer_vulkan>(buffer)->buffer();
    buffer_info.offset = 0;
    buffer_info.range = buffer->size();

    VkWriteDescriptorSet write_info;
    write_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_info.pNext = nullptr;
    write_info.dstBinding = static_cast<std::uint32_t>(slot);
    write_info.dstArrayElement = 0;
    write_info.descriptorCount = 1;
    write_info.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_info.pBufferInfo = &buffer_info;
    write_info.pImageInfo = nullptr;
    write_info.pTexelBufferView = nullptr;
    write_info.dstSet = _descriptor_sets[0];

    vkUpdateDescriptorSets(_device, 1, &write_info, 0, nullptr);
}

void shader_data_vulkan::bind_resource(std::size_t slot, const std::shared_ptr<texture>& texture) {
    RB_ASSERT(texture, "Texture is not provided");

    RB_MAYBE_UNUSED const auto binding = shader()->binding(slot);
    RB_ASSERT(binding.has_value(), "No binding with provided slot index");
    RB_ASSERT(binding.value().binding_type == shader_binding_type::texture, "Binding is not combined texture sampler type");

    auto native_texture = std::static_pointer_cast<texture_vulkan>(texture);

    VkDescriptorImageInfo image_info;
    image_info.imageView = native_texture->image_view();
    image_info.sampler = native_texture->sampler();
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write_info;
    write_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_info.pNext = nullptr;
    write_info.dstBinding = static_cast<std::uint32_t>(slot);
    write_info.dstArrayElement = 0;
    write_info.descriptorCount = 1;
    write_info.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_info.pBufferInfo = nullptr;
    write_info.pImageInfo = &image_info;
    write_info.pTexelBufferView = nullptr;
    write_info.dstSet = _descriptor_sets[0];

    vkUpdateDescriptorSets(_device, 1, &write_info, 0, nullptr);
}

const std::vector<VkDescriptorSet>& shader_data_vulkan::descriptor_sets() const RB_NOEXCEPT {
    return _descriptor_sets;
}

void shader_data_vulkan::_create_descriptor_pool(const shader_data_desc& desc, const std::vector<shader_binding_desc>& bindings) {
    auto pool_sizes = std::make_unique<VkDescriptorPoolSize[]>(bindings.size());
    for (auto index : rb::make_range<std::size_t>(0u, bindings.size())) {
        pool_sizes[index].type = utils_vulkan::descriptor_type(bindings[index].binding_type);
        pool_sizes[index].descriptorCount = 1;
    }

    // todo: compress pool sizes

    VkDescriptorPoolCreateInfo descriptor_pool_info;
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.pNext = nullptr;
    descriptor_pool_info.flags = 0;
    descriptor_pool_info.maxSets = static_cast<std::uint32_t>(_descriptor_sets.size());
    descriptor_pool_info.poolSizeCount = static_cast<std::uint32_t>(bindings.size());
    descriptor_pool_info.pPoolSizes = pool_sizes.get();

    RB_MAYBE_UNUSED auto result = vkCreateDescriptorPool(_device, &descriptor_pool_info, nullptr, &_descriptor_pool);
    RB_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan descriptol pool");
}

void shader_data_vulkan::_create_descriptor_sets(const shader_data_desc& desc) {
    auto native_shader = std::static_pointer_cast<shader_vulkan>(shader());

    std::vector<VkDescriptorSetLayout> layouts(_descriptor_sets.size(), native_shader->descriptor_set_layout());

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = _descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1; // static_cast<std::uint32_t>(_descriptor_sets.size());
    descriptor_set_allocate_info.pSetLayouts = layouts.data();

    RB_MAYBE_UNUSED auto result = vkAllocateDescriptorSets(_device, &descriptor_set_allocate_info, _descriptor_sets.data());
    RB_ASSERT(result == VK_SUCCESS, "Failed to allocate Vulkan descriptor sets");
}

void shader_data_vulkan::_update_descriptor_sets(const shader_data_desc& desc, const std::vector<shader_binding_desc>& bindings) {
   // auto container = std::make_unique<VkWriteDescriptorSet[]>(bindings.size());
    //auto buffer_infos = std::make_unique<VkDescriptorBufferInfo[]>(bindings.size());
    //auto image_infos = std::make_unique<VkDescriptorImageInfo[]>(bindings.size());

    for (auto index : rb::make_range<std::size_t>(0u, bindings.size())) {
        const auto& binding = bindings.at(index);
        if (desc.data.find(binding.slot) == desc.data.end()) {
            continue;
        }

        const auto& parameter = desc.data.at(binding.slot);

        VkWriteDescriptorSet write_info;
        VkDescriptorBufferInfo buffer_info;
        VkDescriptorImageInfo image_info;

        write_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_info.pNext = nullptr;
        write_info.dstBinding = static_cast<std::uint32_t>(binding.slot);
        write_info.dstArrayElement = 0;
        write_info.descriptorCount = 1;
        write_info.descriptorType = utils_vulkan::descriptor_type(binding.binding_type);
        write_info.pBufferInfo = nullptr;
        write_info.pImageInfo = nullptr;
        write_info.pTexelBufferView = nullptr;
        write_info.dstSet = _descriptor_sets[0];

        switch (binding.binding_type) {
            case shader_binding_type::uniform_buffer: {
                auto buffer = std::get<std::shared_ptr<rb::buffer>>(parameter);

                buffer_info.buffer = std::static_pointer_cast<buffer_vulkan>(buffer)->buffer();
                buffer_info.offset = 0;
                buffer_info.range = buffer->size();

                write_info.pBufferInfo = &buffer_info;
                break;
            }
            case shader_binding_type::texture: {
                auto texture = std::get<std::shared_ptr<rb::texture>>(parameter);

                image_info.sampler = std::static_pointer_cast<texture_vulkan>(texture)->sampler();
                image_info.imageView = std::static_pointer_cast<texture_vulkan>(texture)->image_view();
                image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                // image_info.sampler = nullptr;
                // image_info.imageView = nullptr;
                // image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                write_info.pImageInfo = &image_info;
                break;
            }
        }
        vkUpdateDescriptorSets(_device, 1, &write_info, 0, nullptr);
    }

    // vkUpdateDescriptorSets(_device, static_cast<std::uint32_t>(bindings.size()), container.get(), 0, nullptr);
}
