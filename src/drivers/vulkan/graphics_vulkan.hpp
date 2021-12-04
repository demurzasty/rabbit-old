#pragma once 

#include <rabbit/graphics/graphics.hpp>
#include <rabbit/math/vec4.hpp>
#include <rabbit/math/mat4.hpp>

#include <volk.h>
#include <vk_mem_alloc.h>

#include <vector>

namespace rb {
    class graphics_vulkan : public graphics_impl {
        struct alignas(16) main_data {
            mat4f view_matrix{ mat4f::identity() };
            mat4f projection_matrix{ mat4f::identity() };
            mat4f projection_view_matrix{ mat4f::identity() };
            mat4f inverse_projection_view_matrix{ mat4f::identity() };
            mat4f last_projection_view_matrix{ mat4f::identity() };
            vec4f camera_position{ vec4f::zero() };
        };

        struct alignas(16) instance_data {
            mat4f world_matrix{ mat4f::identity() };
            mat4f normal_matrix{ mat4f::identity() };
        };

    public:
        graphics_vulkan();

        ~graphics_vulkan();

        void set_camera_projection_matrix(const mat4f& projection_matrix) override;

        void set_camera_view_matrix(const mat4f& view_matrix) override;

        void set_camera_world_matrix(const mat4f& world_matrix) override;

        instance make_instance() override;

        void destroy_instance(instance instance) override;

        void set_instance_mesh(instance instance, const std::shared_ptr<mesh>& mesh) override;

        void set_instance_material(instance instance, const std::shared_ptr<material>& material) override;

        void set_instance_world_matrix(instance instance, const mat4f& world_matrix) override;

        void draw() override;

        void swap_buffers() override;

    private:
        void _initialize_volk();

        void _create_instance();

        void _choose_physical_device();

        void _create_surface();

        void _create_device();

        void _create_allocator();

        void _query_surface();

        void _create_swapchain();

        void _create_command_pool();

        void _create_synchronization_objects();

        void _create_main_buffer();

        void _create_instance_buffer();

    private:
        VkInstance _instance;
        VkPhysicalDevice _physical_device;
        VkPhysicalDeviceProperties _physical_device_properties;
        VkSurfaceKHR _surface;
        VkDevice _device;

        std::uint32_t _graphics_family;
        std::uint32_t _present_family;
        VkQueue _graphics_queue;
        VkQueue _present_queue;

        VmaAllocator _allocator;

        VkSurfaceFormatKHR _surface_format;
        VkExtent2D _swapchain_extent;
        VkSwapchainKHR _swapchain;
        VkPresentModeKHR _present_mode;

        VkImage _swapchain_depth_image;
        VmaAllocation _swapchain_depth_image_allocation;
        VkImageView _swapchain_depth_image_view;

        std::vector<VkImage> _swapchain_images;
        std::vector<VkImageView> _swapchain_image_views;

        VkRenderPass _swapchain_render_pass;

        std::vector<VkFramebuffer> _swapchain_framebuffers;

        VkCommandPool _command_pool;

        VkSemaphore _render_semaphore;
        VkSemaphore _present_semaphore;

        std::uint32_t _image_index{ 0 };

        main_data _main_data;

        VkBuffer _main_buffer;
        VmaAllocation _main_buffer_allocation;

        VkBuffer _instance_buffer;
        VmaAllocation _instance_buffer_allocation;
    };
}