#pragma once 

#include <rabbit/window.hpp>
#include <rabbit/application.hpp>
#include <rabbit/graphics_device.hpp>

#include <volk.h>
#include <vk_mem_alloc.h>

#include <vector>

namespace rb {
    class graphics_device_vulkan : public graphics_device {
        static constexpr std::uint32_t max_frames_in_flight = 2;

    public:
        graphics_device_vulkan(application_config& config, window& window);

        ~graphics_device_vulkan();

        std::shared_ptr<buffer> make_buffer(const buffer_desc& desc) override;

        std::shared_ptr<mesh> make_mesh(const mesh_desc& desc) override;

        std::shared_ptr<material> make_material(const material_desc& desc) override;

        std::shared_ptr<resource_heap> make_resource_heap(const resource_heap_desc& desc) override;

        void begin() override;

        void end() override;

        void begin_render_pass() override;

        void end_render_pass() override;

        void update_buffer(const std::shared_ptr<buffer>& buffer, const void* data, std::size_t offset, std::size_t size) override;

        void set_viewport(const vec4i& rect) override;

        void draw(const std::shared_ptr<mesh>& mesh, const std::shared_ptr<material>& material, const std::shared_ptr<resource_heap>& resource_heap) override;

        void present() override;

    private:
        VkInstance _instance{ VK_NULL_HANDLE };
        VkPhysicalDevice _physical_device{ VK_NULL_HANDLE };
        VkDevice _device{ VK_NULL_HANDLE };
        VmaAllocator _allocator{ VK_NULL_HANDLE };
        VkSurfaceKHR _surface{ VK_NULL_HANDLE };
        VkSurfaceFormatKHR _surface_format;
        VkSwapchainKHR _swapchain{ VK_NULL_HANDLE };
        VkPresentModeKHR _present_mode;
        VkExtent2D _swapchain_extent;
        VkQueue _graphics_queue{ VK_NULL_HANDLE };
        VkQueue _present_queue{ VK_NULL_HANDLE };
        std::vector<VkImage> _images;
        std::vector<VkImageView> _image_views;
        VkRenderPass _render_pass{ VK_NULL_HANDLE };
        std::vector<VkFramebuffer> _framebuffers;
        VkDescriptorPool _descriptor_pool{ VK_NULL_HANDLE };
        VkCommandPool _command_pool{ VK_NULL_HANDLE };
        VkCommandBuffer _command_buffer{ VK_NULL_HANDLE };
        VkSemaphore _render_semaphore{ VK_NULL_HANDLE };
        VkSemaphore _present_semaphore{ VK_NULL_HANDLE };
        VkFence _render_fence{ VK_NULL_HANDLE };
        std::uint32_t _current_frame{ 0 };
        std::uint32_t _image_index{ 0 };
    };
}
