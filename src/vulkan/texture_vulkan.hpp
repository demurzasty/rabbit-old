#pragma once 

#include <rabbit/texture.hpp>

#include <volk.h>
#include <vk_mem_alloc.h>

namespace rb {
    class texture_vulkan : public texture {
    public:
        texture_vulkan(VkDevice device, VkQueue graphics_queue, VkCommandPool command_pool, VmaAllocator allocator, const texture_desc& desc);

        ~texture_vulkan();

        RB_NODISCARD VkImage image() const RB_NOEXCEPT;

        RB_NODISCARD VkImageView image_view() const RB_NOEXCEPT;
        
        RB_NODISCARD VkSampler sampler() const RB_NOEXCEPT;
    
    private:
        void _create_image(const texture_desc& desc);

        void _update_image(VkQueue graphics_queue, VkCommandPool command_pool, const texture_desc& desc);

        void _create_image_view(const texture_desc& desc);

        void _create_sampler(const texture_desc& desc);

        void _transition_image_layout(VkQueue graphics_queue, VkCommandPool command_pool, VkImageLayout old_layout, VkImageLayout new_layout);

        VkCommandBuffer _begin_single_time_commands(VkCommandPool command_pool);

        void _end_single_time_commands(VkQueue graphics_queue, VkCommandPool command_pool, VkCommandBuffer command_buffer);

    private:
        VkDevice _device;
        VmaAllocator _allocator;
        VmaAllocation _allocation;
        VkImage _image;
        VkImageView _image_view;
        VkSampler _sampler;
    };
}
