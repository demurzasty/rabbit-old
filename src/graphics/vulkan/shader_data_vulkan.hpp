#pragma once

#include <rabbit/graphics/shader_data.hpp>

#include <volk.h>

namespace rb {
    class shader_data_vulkan : public shader_data {
    public:
        shader_data_vulkan(VkDevice device, const shader_data_desc& desc);

        ~shader_data_vulkan();

        void bind_resource(std::size_t slot, const std::shared_ptr<buffer>& buffer) override;

        void bind_resource(std::size_t slot, const std::shared_ptr<texture>& buffer) override;

        RB_NODISCARD const std::vector<VkDescriptorSet>& descriptor_sets() const RB_NOEXCEPT;

    private:
        void _create_descriptor_pool(const shader_data_desc& desc, const std::vector<shader_binding_desc>& bindings);

        void _create_descriptor_sets(const shader_data_desc& desc);

        void _update_descriptor_sets(const shader_data_desc& desc, const std::vector<shader_binding_desc>& bindings);

    private:
        VkDevice _device;
        VkDescriptorPool _descriptor_pool;
        std::vector<VkDescriptorSet> _descriptor_sets;
    };
}
