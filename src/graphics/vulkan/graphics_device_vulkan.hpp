#pragma once 

#include <rabbit/graphics/graphics_device.hpp>

#include "buffer_vulkan.hpp"

#include <volk.h>

#include <vector>

namespace rb {
    class graphics_device_vulkan : public graphics_device {
    public:
        graphics_device_vulkan(const config& config, window& window);

        ~graphics_device_vulkan();

        std::shared_ptr<texture> make_texture(const texture_desc& desc) override;

        std::shared_ptr<texture_cube> make_texture(const texture_cube_desc& texture_desc) override;

        std::shared_ptr<buffer> make_buffer(const buffer_desc& buffer_desc) override;

        std::shared_ptr<shader> make_shader(const shader_desc& shader_desc) override;

        std::shared_ptr<mesh> make_mesh(const mesh_desc& mesh_desc) override;

        void clear(const color& color) override;

        void present() override;

        void set_blend_state(const blend_state& blend_state) override;

        void set_depth_test(bool depth_test) override;

        void set_backbuffer_size(const vec2i& size) override;

        vec2i backbuffer_size() const override;

        void set_clip_rect(const vec4i& clip_rect) override;

        void set_render_target(const std::shared_ptr<texture>& render_target) override;

        void set_render_target(const std::shared_ptr<texture_cube>& render_target, texture_cube_face face, int level) override;

        void bind_buffer_base(const std::shared_ptr<buffer>& buffer, std::size_t binding_index) override;

        void bind_texture(const std::shared_ptr<texture>& texture, std::size_t binding_index) override;
        
        void bind_texture(const std::shared_ptr<texture_cube>& texture, std::size_t binding_index) override;

        void draw(const std::shared_ptr<mesh>& mesh, const std::shared_ptr<shader>& shader) override;

    private:
        struct swap_chain_buffer {
            VkImage image;
            VkImageView view;
        };

    private:
        VkInstance _instance{ VK_NULL_HANDLE };
        VkPhysicalDevice _physical_device{ VK_NULL_HANDLE };
        VkDevice _device{ VK_NULL_HANDLE };
        VkSurfaceKHR _surface{ VK_NULL_HANDLE };
        VkFormat _color_format;
        VkColorSpaceKHR _color_space;
        VkSwapchainKHR _swap_chain;
        VkPresentModeKHR _present_mode;
        VkExtent2D _swap_extent;
        VkQueue _graphics_queue;
        VkQueue _present_queue;
        std::vector<VkImage> _images;
        std::vector<VkImageView> _image_views;
        VkRenderPass _render_pass;
        VkPipelineLayout _pipeline_layout;
        VkPipeline _pipeline;
        std::vector<VkFramebuffer> _framebuffers;
        VkCommandPool _command_pool;
        std::vector<VkCommandBuffer> _command_buffers;

        std::vector<VkSemaphore> _image_available_semaphores;
        std::vector<VkSemaphore> _render_finished_semaphores;
        std::vector<VkFence> _in_flight_fences;
        std::vector<VkFence> _images_in_flight;

        std::shared_ptr<buffer_vulkan> _vertex_buffer2;

        std::size_t _current_frame = 0;
    };
}
