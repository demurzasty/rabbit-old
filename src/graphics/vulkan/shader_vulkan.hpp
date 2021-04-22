#pragma once

#include <rabbit/graphics/shader.hpp>

#include <volk.h>

namespace rb {
    class shader_vulkan : public shader {
    public:
        shader_vulkan(VkDevice device, VkRenderPass render_pass, VkExtent2D swapchain_extent, const shader_desc& desc);

        ~shader_vulkan();

        RB_NODISCARD VkDescriptorSetLayout descriptor_set_layout() const RB_NOEXCEPT;

        RB_NODISCARD VkPipelineLayout pipeline_layout() const RB_NOEXCEPT;

        RB_NODISCARD VkPipeline pipeline() const RB_NOEXCEPT;

    private:
        void _create_descriptor_set_layout(const shader_desc& desc);

        void _create_pipeline_layout(const shader_desc& desc);

        RB_NODISCARD VkShaderModule _create_shader_module(const span<const std::uint32_t>& bytecode);

        void _create_pipeline(const span<const VkPipelineShaderStageCreateInfo>& shader_stages, VkRenderPass render_pass, VkExtent2D swapchain_extent, const shader_desc& desc);

    private:
        VkDevice _device;
        VkDescriptorSetLayout _descriptor_set_layout;
        VkPipelineLayout _pipeline_layout;
        VkPipeline _pipeline;
    };
}
