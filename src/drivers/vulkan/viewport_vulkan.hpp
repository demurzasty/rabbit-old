#pragma once 

#include <rabbit/viewport.hpp>
#include <rabbit/vec3.hpp>
#include <rabbit/vec4.hpp>

#include <volk.h>
#include <vk_mem_alloc.h>

#include <atomic>

namespace rb {
    // 1. Depth Pre-Pass
    // 2. Forward pass (with depth comparison set to equal)
    // 3. Postprocesses (uber shader)

    class viewport_vulkan : public viewport {
        struct alignas(16) light_data {
            vec4f position_or_direction;
            vec4f color_and_radius;
        };

        struct visible_index {
            int index;
        };

        struct cull_data {
            unsigned int light_count;
            unsigned int number_of_tiles_x;
        };

    public:
        viewport_vulkan(VkDevice device,
            VmaAllocator allocator,
            VkFormat depth_format,
            VkRenderPass depth_render_pass,
            VkDescriptorSetLayout depth_descriptor_set_layout,
            VkDescriptorSetLayout light_descriptor_set_layout,
            VkRenderPass forward_render_pass,
            VkDescriptorSetLayout forward_descriptor_set_layout,
            VkRenderPass postprocess_render_pass,
            VkDescriptorSetLayout postprocess_descriptor_set_layout,
            VkRenderPass fill_render_pass,
            VkDescriptorSetLayout fill_descriptor_set_layout,
            const viewport_desc& desc);

        ~viewport_vulkan();

        void begin_depth_pass(VkCommandBuffer command_buffer);

        void end_depth_pass(VkCommandBuffer command_buffer);

        void begin_light_pass(VkCommandBuffer command_buffer);

        void add_point_light(const vec3f& position, float radius, const vec3f& color);

        void add_directional_light(const vec3f& direction, const vec3f& color, bool shadow_enabled);

        void end_light_pass(VkCommandBuffer command_buffer);

        void begin_forward_pass(VkCommandBuffer command_buffer);

        void end_forward_pass(VkCommandBuffer command_buffer);

        void begin_postprocess_pass(VkCommandBuffer command_buffer);

        void end_postprocess_pass(VkCommandBuffer command_buffer);

        VkDescriptorSet depth_descriptor_set() const;

        VkDescriptorSet light_descriptor_set() const;

        VkBuffer light_buffer() const;

        VkBuffer visible_light_indices_buffer() const;

        VkDescriptorSet forward_descriptor_set() const;

        VkDescriptorSet postprocess_descriptor_set() const;

        VkDescriptorSet fill_descriptor_set() const;

        VkFramebuffer fill_framebuffer() const;

        bool has_shadows() const;

    private:
        void _create_descriptor_pool(const viewport_desc& desc);

        void _create_sampler(const viewport_desc& desc);

        void _create_depth(const viewport_desc& desc, VkFormat depth_format);

        void _create_light(const viewport_desc& desc);

        void _create_forward(const viewport_desc& desc);

        void _create_postprocess(const viewport_desc& desc);

        void _create_fill(VkRenderPass fill_render_pass, VkDescriptorSetLayout fill_descriptor_set_layout, const viewport_desc& desc);

    private:
        VkDevice _device;
        VmaAllocator _allocator;
        VkRenderPass _depth_render_pass;
        VkDescriptorSetLayout _depth_descriptor_set_layout;
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
		VkFramebuffer _depth_framebuffer;
        VkDescriptorSet _depth_descriptor_set;

        VkDescriptorSet _light_descriptor_set;
        VkBuffer _light_buffer;
        VmaAllocation _light_buffer_allocation;
        VkBuffer _visible_light_indices_buffer;
        VmaAllocation _visible_light_indices_buffer_allocation;
        VkBuffer _light_info_buffer;
        VmaAllocation _light_info_buffer_allocation;

        light_data* _light_data;
        std::atomic<std::size_t> _light_index{ 0 };

        bool _has_shadows{ false };

        VkImage _forward_image; // also as composition image as first pass
        VmaAllocation _forward_image_allocation;
        VkImageView _forward_image_view;
        VkFramebuffer _forward_framebuffer;
        VkDescriptorSet _forward_descriptor_set; // contains composition image

        VkImage _postprocess_images[2];
        VmaAllocation _postprocess_image_allocations[2];
        VkImageView _postprocess_image_views[2];
        VkFramebuffer _postprocess_framebuffers[2];
        VkDescriptorSet _postprocess_descriptor_sets[2];
        std::size_t _current_postprocess_image_index{ 0 };

        VkImage _fill_image;
        VmaAllocation _fill_image_allocation;
        VkImageView _fill_image_view;
        VkFramebuffer _fill_framebuffer;
        VkDescriptorSet _fill_descriptor_set;
    };
}
