#pragma once 

#include <rabbit/graphics.hpp>
#include <rabbit/window.hpp>
#include <rabbit/settings.hpp>
#include <rabbit/mat4.hpp>
#include <rabbit/vec3.hpp>
#include <rabbit/vec4.hpp>
#include <rabbit/math.hpp>

#include "environment_vulkan.hpp"

#include <volk.h>
#include <vk_mem_alloc.h>

#include <vector>

namespace rb {
	class graphics_vulkan : public graphics_impl {
	public:
		struct alignas(16) camera_data {
			mat4f projection;
			mat4f view;
			mat4f inv_proj_view;
			vec3f camera_position;
		};

		struct alignas(16) light_data {
			vec3f dir_or_pos;
			float radius;
			vec3f color;
			int type;
		};

		struct alignas(16) light_list_data {
			light_data lights[graphics_limits::max_lights];
			mat4f light_proj_view;
			int light_count;
		};

		struct alignas(16) local_data {
			mat4f world;
		};

		struct alignas(16) irradiance_data {
			int cube_face;
		};

		struct alignas(16) prefilter_data {
			int cube_face;
			float roughness;
		};

		struct alignas(16) shadow_data {
			mat4f proj_view_world;
		};

		struct alignas(16) directional_light_data {
			vec3f light_dir; float padding[1];
			vec3f light_color; float padding2[1];
			mat4f light_proj_view;
		};

	public:
		graphics_vulkan();

		~graphics_vulkan();

		std::shared_ptr<texture> make_texture(const texture_desc& desc) override;

		std::shared_ptr<environment> make_environment(const environment_desc& desc) override;

		std::shared_ptr<material> make_material(const material_desc& desc) override;

		std::shared_ptr<mesh> make_mesh(const mesh_desc& desc) override;

		void begin() override;

		void set_camera(const transform& transform, const camera& camera) override;

		void begin_geometry_pass() override;

		void draw_geometry(const transform& transform, const geometry& geometry) override;

		void end_geometry_pass() override;

		void begin_shadow_pass(const transform& transform, const light& light, const directional_light& directional_light) override;

		void draw_shadow(const transform& transform, const geometry& geometry) override;

		void end_shadow_pass() override;

		void begin_render_pass() override;

		void draw_ambient() override;

		void draw_directional_light(const transform& transform, const light& light, const directional_light& directional_light) override;

		void draw_skybox() override;

		void end_render_pass() override;

		void end() override;

		void present() override;

		void flush() override;

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

		void _create_quad();

		void _generate_brdf_image();

		void _create_camera_buffer();

		void _create_skybox();

		void _create_irradiance_pipeline();

		void _bake_irradiance(const std::shared_ptr<environment>& environment);

		void _create_prefilter_pipeline();

		void _bake_prefilter(const std::shared_ptr<environment>& environment);

		void _create_shadow_map();

		void _create_forward_pipeline();

		void _create_gbuffer();

		void _create_ambient_pipeline();

		void _create_directional_light_pipeline();

		void _create_skybox_pipeline();

		void _create_command_buffers();

		VkCommandBuffer _command_begin();

		void _command_end();

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

		VkImage _depth_image;
		VmaAllocation _depth_image_allocation;
		VkImageView _depth_image_view;

		std::vector<VkImage> _images;
		std::vector<VkImageView> _image_views;
		std::vector<VkFramebuffer> _framebuffers;

		VkRenderPass _render_pass;
		VkRenderPass _second_render_pass;

		VkCommandPool _command_pool;

		VkSemaphore _render_semaphore;
		VkSemaphore _present_semaphore;

		std::uint32_t _image_index{ 0 };

		VkBuffer _quad_vertex_buffer;
		VmaAllocation _quad_vertex_allocation;

		VkBuffer _quad_index_buffer;
		VmaAllocation _quad_index_allocation;

		VmaAllocation _brdf_allocation;
		VkImage _brdf_image;
		VkImageView _brdf_image_view;
		VkSampler _brdf_sampler;

		VkBuffer _camera_buffer;
		VmaAllocation _camera_allocation;

		VkBuffer _light_buffer;
		VmaAllocation _light_allocation;

		camera_data _camera_data;

		VkBuffer _skybox_vertex_buffer;
		VmaAllocation _skybox_vertex_allocation;

		VkBuffer _skybox_index_buffer;
		VmaAllocation _skybox_index_allocation;

		VkDescriptorPool _irradiance_descriptor_pool;
		VkDescriptorSet _irradiance_descriptor_set;
		VkDescriptorSetLayout _irradiance_descriptor_set_layout;
		VkPipelineLayout _irradiance_pipeline_layout;
		VkShaderModule _irradiance_shader_modules[2];
		VkRenderPass _irradiance_render_pass;
		VkPipeline _irradiance_pipeline;

		VkDescriptorPool _prefilter_descriptor_pool;
		VkDescriptorSet _prefilter_descriptor_set;
		VkDescriptorSetLayout _prefilter_descriptor_set_layout;
		VkPipelineLayout _prefilter_pipeline_layout;
		VkShaderModule _prefilter_shader_modules[2];
		VkRenderPass _prefilter_render_pass;
		VkPipeline _prefilter_pipeline;

		VkImage _shadow_image;
		VkImageView _shadow_image_view;
		VmaAllocation _shadow_allocation;
		VkSampler _shadow_sampler;
		VkRenderPass _shadow_render_pass;
		VkFramebuffer _shadow_framebuffer;
		VkPipelineLayout _shadow_pipeline_layout;
		VkShaderModule _shadow_shader_module;
		VkPipeline _shadow_pipeline;

		VkDescriptorPool _forward_descriptor_pool;
		VkDescriptorSet _forward_descriptor_set;
		VkDescriptorSetLayout _forward_descriptor_set_layout[3];
		VkPipelineLayout _forward_pipeline_layout;
		VkShaderModule _forward_shader_modules[2];
		VkPipeline _forward_pipeline;

		VkImage _gbuffer[3];
		VkImageView _gbuffer_views[3];
		VmaAllocation _gbuffer_allocations[3];
		VkSampler _gbuffer_sampler;
		VkRenderPass _gbuffer_render_pass;
		VkFramebuffer _gbuffer_framebuffer;
		VkDescriptorPool _gbuffer_descriptor_pool;
		VkDescriptorSet _gbuffer_descriptor_set;
		VkDescriptorSetLayout _gbuffer_descriptor_set_layout;
		VkPipelineLayout _gbuffer_pipeline_layout;
		VkShaderModule _gbuffer_shader_modules[2];
		VkPipeline _gbuffer_pipeline;

		VkPipelineLayout _ambient_pipeline_layout;
		VkShaderModule _ambient_shader_modules[2];
		VkPipeline _ambient_pipeline;

		VkPipelineLayout _directional_light_pipeline_layout;
		VkPipeline _directional_light_pipeline;

		VkPipelineLayout _skybox_pipeline_layout;
		VkShaderModule _skybox_shader_modules[2];
		VkPipeline _skybox_pipeline;

		VkCommandBuffer _command_buffers[3];
		VkFence _fences[3];
		std::size_t _command_index{ 0 };

		bool _first_render_pass{ false };
		mat4f _light_proj_view;
		std::shared_ptr<environment_vulkan> _environment;
	};
}
