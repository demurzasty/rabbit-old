#pragma once 

#include <rabbit/viewport.hpp>

#include <volk.h>
#include <vk_mem_alloc.h>

namespace rb {
    // 1. G-Buffer generation
    // 2. Light accumulation buffer generation
    // 3. Result image composition
    // 4. Post-processing rendering

    class viewport_vulkan : public viewport {
    public:
        viewport_vulkan(VkDevice device,
            VmaAllocator allocator,
            VkRenderPass gbuffer_render_pass,
            VkDescriptorSetLayout gbuffer_descriptor_set_layout,
            VkRenderPass light_render_pass,
            VkDescriptorSetLayout light_descriptor_set_layout,
            VkRenderPass forward_render_pass,
            VkDescriptorSetLayout forward_descriptor_set_layout,
            VkRenderPass postprocess_render_pass,
            VkDescriptorSetLayout postprocess_descriptor_set_layout,
            const viewport_desc& desc);

        ~viewport_vulkan();

        void begin_geometry_pass(VkCommandBuffer command_buffer);

        void end_geometry_pass(VkCommandBuffer command_buffer);

        void begin_light_pass(VkCommandBuffer command_buffer);

        void end_light_pass(VkCommandBuffer command_buffer);

        void begin_forward_pass(VkCommandBuffer command_buffer);

        void end_forward_pass(VkCommandBuffer command_buffer);

        void begin_postprocess_pass(VkCommandBuffer command_buffer);

        void end_postprocess_pass(VkCommandBuffer command_buffer);

        VkDescriptorSet gbuffer_descriptor_set() const;

        VkDescriptorSet light_descriptor_set() const;

        VkDescriptorSet forward_descriptor_set() const;

        VkDescriptorSet last_postprocess_descriptor_set() const;

    private:
        void _create_descriptor_pool(const viewport_desc& desc);

        void _create_sampler(const viewport_desc& desc);

        void _create_depth(const viewport_desc& desc);

        void _create_gbuffer(const viewport_desc& desc);

        void _create_light(const viewport_desc& desc);

        void _create_forward(const viewport_desc& desc);

        void _create_postprocess(const viewport_desc& desc);

    private:
        VkDevice _device;
        VmaAllocator _allocator;
        VkRenderPass _gbuffer_render_pass;
        VkDescriptorSetLayout _gbuffer_descriptor_set_layout;
        VkRenderPass _light_render_pass;
        VkDescriptorSetLayout _light_descriptor_set_layout;
        VkRenderPass _forward_render_pass;
        VkDescriptorSetLayout _forward_descriptor_set_layout;
        VkRenderPass _postprocess_render_pass;
        VkDescriptorSetLayout _postprocess_descriptor_set_layout;

        VkDescriptorPool _descriptor_pool;

        VkSampler _sampler; // basic nearest/clamp-to-edge sampler

        VkImage _depth_image;
        VmaAllocation _depth_image_allocation;
        VkImageView _depth_image_view;

		VkImage _gbuffer_images[3];
        VmaAllocation _gbuffer_images_allocations[3];
		VkImageView _gbuffer_images_views[3];
		VkFramebuffer _gbuffer_framebuffer;
        VkDescriptorSet _gbuffer_descriptor_set; // contains 3 images from g-buffer + 1 depth buffer

        VkImage _light_image; // light accumulation buffer (no depth buffer)
        VmaAllocation _light_image_allocation;
        VkImageView _light_image_view;
        VkFramebuffer _light_framebuffer;
        VkDescriptorSet _light_descriptor_set; // contains light image

        VkImage _forward_image; // also as composition image as first pass
        VmaAllocation _forward_image_allocation;
        VkImageView _forward_image_view;
        VkFramebuffer _forward_framebuffer;
        VkDescriptorSet _forward_descriptor_set; // contains composition image

        VkImage _postprocess_images[2];
        VmaAllocation _postprocess_images_allocations[2];
        VkImageView _postprocess_images_views[2];
        VkFramebuffer _postprocess_framebuffers[2];
        VkDescriptorSet _postprocess_descriptor_sets[2];

        std::size_t _current_postprocess_image_index{ 0 };
    };
}
