#pragma once 

#include <rabbit/material.hpp>

#include <volk.h>

namespace rb {
    class material_vulkan : public material {
    public:
        material_vulkan(VkDevice device, VkDescriptorPool descriptor_pool, VkRenderPass render_pass, VkExtent2D swapchain_extent, const material_desc& desc);

        ~material_vulkan();

        RB_NODISCARD VkDescriptorSetLayout descriptor_set_layout() const RB_NOEXCEPT;

        RB_NODISCARD VkPipelineLayout pipeline_layout() const RB_NOEXCEPT;

        RB_NODISCARD VkPipeline pipeline() const RB_NOEXCEPT;

    private:
        void _create_descriptor_set_layout(const material_desc& desc);

        void _create_pipeline_layout(const material_desc& desc);

        RB_NODISCARD VkShaderModule _create_shader_module(const span<const std::uint32_t>& bytecode);

        void _create_descriptor_set(const material_desc& desc);

        void _create_pipeline(const span<const VkPipelineShaderStageCreateInfo>& shader_stages, VkRenderPass render_pass, VkExtent2D swapchain_extent, const material_desc& desc);

    private:
        VkDevice _device;
        VkDescriptorPool _descriptor_pool;

        VkDescriptorSetLayout _descriptor_set_layout;
        VkPipelineLayout _pipeline_layout;
        VkDescriptorSet _descriptor_set;
        VkPipeline _pipeline;
    };
}
