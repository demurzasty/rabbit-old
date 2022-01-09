#pragma once 

#include <rabbit/graphics/graphics.hpp>
#include <rabbit/platform/window.hpp>
#include <rabbit/core/settings.hpp>
#include <rabbit/math/mat4.hpp>
#include <rabbit/math/vec3.hpp>
#include <rabbit/math/vec4.hpp>
#include <rabbit/math/math.hpp>

#include <volk.h>
#include <vk_mem_alloc.h>

#include <vector>
#include <unordered_map>

namespace rb {
	class graphics_base_vulkan : public graphics_impl {
	public:
		static constexpr std::size_t max_command_buffers{ 3 };

	public:
		graphics_base_vulkan();

		virtual ~graphics_base_vulkan();

		virtual std::shared_ptr<texture> make_texture(const texture_desc& desc) override;

		virtual std::shared_ptr<environment> make_environment(const environment_desc& desc) override;

		virtual std::shared_ptr<material> make_material(const material_desc& desc) override;

		virtual void present() override;

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
		
		void _create_command_buffers();

	protected:
		VkFormat _get_supported_depth_format();

		VkFormat _get_supported_shadow_format();

		bool _is_format_supported(VkFormat format, VkFormatFeatureFlags flags);

		void _create_buffer(VkBufferUsageFlags usage, VmaMemoryUsage memory_usage, std::uint32_t size, VkBuffer* buffer, VmaAllocation* allocation);

		void _create_descriptor_set_layout(const span<const VkDescriptorSetLayoutBinding>& bindings, VkDescriptorSetLayoutCreateFlags flags, VkDescriptorSetLayout* layout);
	
		void _create_shader_module_from_code(shader_stage stage, const span<const std::uint8_t>& code, const std::vector<std::string>& definitions, VkShaderModule* module);

		void _create_shader_module(const span<const std::uint32_t>& spirv, VkShaderModule* module);

		void _create_compute_pipeline(VkShaderModule shader_module, VkPipelineLayout layout, VkPipeline* pipeline);

		void _buffer_barrier(VkCommandBuffer command_buffer, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, VkAccessFlags src_access, VkAccessFlags dst_access, VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage);
		
		VkCommandBuffer _command_begin();

		void _command_end();

	protected:
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

		VkImage _depth_image;
		VmaAllocation _depth_image_allocation;
		VkImageView _depth_image_view;

		std::vector<VkImage> _images;
		std::vector<VkImageView> _image_views;
		std::vector<VkFramebuffer> _framebuffers;

		VkRenderPass _render_pass;

		VkCommandPool _command_pool;

		VkSemaphore _render_semaphore;
		VkSemaphore _present_semaphore;

		std::uint32_t _image_index{ 0 };

		VkCommandBuffer _command_buffers[max_command_buffers];
		VkFence _fences[max_command_buffers];
		std::size_t _command_index{ 0 };
	};
}
