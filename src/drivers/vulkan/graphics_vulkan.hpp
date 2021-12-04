#pragma once 

#include <rabbit/graphics/graphics.hpp>
#include <rabbit/platform/window.hpp>
#include <rabbit/core/settings.hpp>
#include <rabbit/math/mat4.hpp>
#include <rabbit/math/vec3.hpp>
#include <rabbit/math/vec4.hpp>
#include <rabbit/math/math.hpp>

#include "environment_vulkan.hpp"

#include <volk.h>
#include <vk_mem_alloc.h>

#include <vector>
#include <unordered_map>

namespace rb {
	struct graphics_vulkan_flags {
		static constexpr std::uint64_t shadow_map_bit{ 1llu << 32llu };
	};

	class graphics_vulkan : public graphics_impl {
	public:
		static constexpr std::size_t max_command_buffers{ 3 };

		struct alignas(16) camera_data {
			mat4f projection;
			mat4f view;
			mat4f inv_proj_view;
			vec3f camera_position; float padding;
			mat4f light_proj_view[4];
			mat4f last_proj_view{ mat4f::identity() };
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
		};

		struct alignas(16) point_light_data {
			vec3f light_position;
			float light_radius;
			vec3f light_color; 
		};

		struct alignas(16) ssao_data {
			vec3f samples[64];
		};

		struct alignas(16) blur_data {
			int strength;
		};

		struct alignas(16) sharpen_data {
			float strength;
		};

	public:
		graphics_vulkan();

		~graphics_vulkan();

		std::shared_ptr<viewport> make_viewport(const viewport_desc& desc) override;

		std::shared_ptr<texture> make_texture(const texture_desc& desc) override;

		std::shared_ptr<environment> make_environment(const environment_desc& desc) override;

		std::shared_ptr<material> make_material(const material_desc& desc) override;

		std::shared_ptr<mesh> make_mesh(const mesh_desc& desc) override;

		void begin() override;

		void set_camera(const mat4f& projection, const mat4f& view, const mat4f& world, const std::shared_ptr<environment>& environment) override;

		void begin_depth_pass(const std::shared_ptr<viewport>& viewport) override;

		void draw_depth(const std::shared_ptr<viewport>& viewport, const mat4f& world, const std::shared_ptr<mesh>& mesh, std::size_t mesh_lod_index) override;

		void end_depth_pass(const std::shared_ptr<viewport>& viewport) override;

		void begin_shadow_pass(const transform& transform, const light& light, const directional_light& directional_light, std::size_t cascade) override;

		void draw_shadow(const mat4f& world, const geometry& geometry, std::size_t cascade) override;

		void end_shadow_pass() override;

		void begin_light_pass(const std::shared_ptr<viewport>& viewport) override;

		void add_point_light(const std::shared_ptr<viewport>& viewport, const transform& transform, const light& light, const point_light& point_light) override;
		
		void add_directional_light(const std::shared_ptr<viewport>& viewport, const transform& transform, const light& light, const directional_light& directional_light, bool use_shadow) override;

		void end_light_pass(const std::shared_ptr<viewport>& viewport) override;

		void begin_forward_pass(const std::shared_ptr<viewport>& viewport) override;

		void draw_skybox(const std::shared_ptr<viewport>& viewport) override;

		void draw_forward(const std::shared_ptr<viewport>& viewport, const mat4f& world, const std::shared_ptr<mesh>& mesh, const std::shared_ptr<material>& material, std::size_t mesh_lod_index) override;

		void end_forward_pass(const std::shared_ptr<viewport>& viewport) override;

		void pre_draw_ssao(const std::shared_ptr<viewport>& viewport) override;

		void begin_fill_pass(const std::shared_ptr<viewport>& viewport) override;

		void draw_fill(const std::shared_ptr<viewport>& viewport, const transform& transform, const geometry& geometry) override;

		void end_fill_pass(const std::shared_ptr<viewport>& viewport) override;

		void begin_postprocess_pass(const std::shared_ptr<viewport>& viewport) override;

		void next_postprocess_pass(const std::shared_ptr<viewport>& viewport) override;

		void draw_ssao(const std::shared_ptr<viewport>& viewport) override;

		void draw_fxaa(const std::shared_ptr<viewport>& viewport) override;

		void draw_blur(const std::shared_ptr<viewport>& viewport, int strength) override;

		void draw_sharpen(const std::shared_ptr<viewport>& viewport, float strength) override;

		void draw_motion_blur(const std::shared_ptr<viewport>& viewport) override;

		void draw_outline(const std::shared_ptr<viewport>& viewport) override;

		void end_postprocess_pass(const std::shared_ptr<viewport>& viewport) override;

		void begin_immediate_pass() override;

		void draw_immediate_color(const span<const vertex>& vertices, const color& color) override;

		void draw_immediate_textured(const span<const vertex>& vertices, const std::shared_ptr<texture>& texture) override;

		void end_immediate_pass() override;

		void end() override;

		void present(const std::shared_ptr<viewport>& viewport) override;

		void swap_buffers() override;

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

		void _create_skybox();

		void _create_irradiance_pipeline();

		void _bake_irradiance(const std::shared_ptr<environment>& environment);

		void _create_prefilter_pipeline();

		void _bake_prefilter(const std::shared_ptr<environment>& environment);

		void _create_shadow_map();

		void _create_camera();

		void _create_main();

		void _create_material();

		void _create_environment();

		void _create_depth();

		void _create_light();

		void _create_forward();

		VkPipelineLayout _create_forward_pipeline_layout(const std::shared_ptr<material>& material);

		VkPipelineLayout _get_forward_pipeline_layout(const std::shared_ptr<material>& material);

		VkPipeline _create_forward_pipeline(const std::shared_ptr<material>& material, std::uint64_t internal_flags);

		VkPipeline _get_forward_pipeline(const std::shared_ptr<material>& material, std::uint64_t internal_flags);

		void _create_postprocess();

		void _create_ssao_pipeline();

		void _create_fxaa_pipeline();

		void _create_blur_pipeline();

		void _create_sharpen_pipeline();

		void _create_motion_blur_pipeline();

		void _create_fill_pipeline();

		void _create_outline_pipeline();

		void _create_skybox_pipeline();

		void _create_present_pipeline();

		void _create_command_buffers();

		VkCommandBuffer _command_begin();

		void _command_end();

		VkFormat _get_supported_depth_format();

		VkFormat _get_supported_shadow_format();

		bool _is_format_supported(VkFormat format, VkFormatFeatureFlags flags);

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
		VkImageView _shadow_image_views[graphics_limits::max_shadow_cascades];
		VmaAllocation _shadow_allocation;
		VkSampler _shadow_sampler;
		VkRenderPass _shadow_render_pass;
		VkFramebuffer _shadow_framebuffers[graphics_limits::max_shadow_cascades];
		VkPipelineLayout _shadow_pipeline_layout;
		VkShaderModule _shadow_shader_module;
		VkPipeline _shadow_pipeline;

		VkBuffer _camera_buffer;
		VmaAllocation _camera_allocation;

		VkDescriptorPool _main_descriptor_pool;
		VkDescriptorSetLayout _main_descriptor_set_layout;
		VkDescriptorSet _main_descriptor_set; // main camera, brdf and shadow map information

		VkDescriptorSetLayout _material_descriptor_set_layout;

		VkDescriptorSetLayout _environment_descriptor_set_layout;

		VkDescriptorSetLayout _depth_descriptor_set_layout;
		VkRenderPass _depth_render_pass;
		VkPipelineLayout _depth_pipeline_layout;
		VkPipeline _depth_pipeline;

		VkDescriptorSetLayout _light_descriptor_set_layout;
		VkPipelineLayout _light_pipeline_layout;
		VkPipeline _light_pipeline;

		VkDescriptorSetLayout _forward_descriptor_set_layout;
		VkRenderPass _forward_render_pass;
		std::unordered_map<std::uint64_t, VkPipelineLayout> _forward_pipeline_layouts;
		std::unordered_map<std::uint64_t, VkPipeline> _forward_pipelines;

		VkDescriptorSetLayout _postprocess_descriptor_set_layout;
		VkRenderPass _postprocess_render_pass;

		VkPipelineLayout _skybox_pipeline_layout;
		VkPipeline _skybox_pipeline;

		VkBuffer _ssao_buffer;
		VmaAllocation _ssao_allocation;
		VkImage _ssao_noise_map;
		VkImageView _ssao_noise_map_view;
		VmaAllocation _ssao_noise_map_allocation;
		VkSampler _ssao_sampler;
		VkImage _ssao_image;
		VmaAllocation _ssao_image_allocation;
		VkImageView _ssao_image_view;
		VkRenderPass _ssao_render_pass;
		VkFramebuffer _ssao_framebuffer;
		VkDescriptorPool _ssao_descriptor_pool;
		VkDescriptorSet _ssao_descriptor_set;
		VkDescriptorSetLayout _ssao_descriptor_set_layout;
		VkPipelineLayout _ssao_pipeline_layout;
		VkPipeline _ssao_pipeline;
		VkDescriptorSetLayout _ssao_blur_descriptor_set_layout;
		VkDescriptorSet _ssao_blur_descriptor_set;
		VkPipelineLayout _ssao_blur_pipeline_layout;
		VkPipeline _ssao_blur_pipeline;

		VkPipelineLayout _fxaa_pipeline_layout;
		VkPipeline _fxaa_pipeline;

		VkPipelineLayout _blur_pipeline_layout;
		VkPipeline _blur_pipelines[2];

		VkPipelineLayout _sharpen_pipeline_layout;
		VkPipeline _sharpen_pipeline;

		VkPipelineLayout _motion_blur_pipeline_layout;
		VkPipeline _motion_blur_pipeline;

		VkRenderPass _fill_render_pass;
		VkDescriptorSetLayout _fill_descriptor_set_layout;
		VkPipelineLayout _fill_pipeline_layout;
		VkPipeline _fill_pipeline;

		VkPipelineLayout _outline_pipeline_layout;
		VkPipeline _outline_pipeline; 

		VkPipelineLayout _present_pipeline_layout;
		VkPipeline _present_pipeline;
		VkPipeline _light_copy_pipeline;
		VkPipelineLayout _forward_copy_pipeline_layout;
		VkPipeline _forward_copy_pipeline;

		VkCommandBuffer _shadow_command_buffer;

		VkCommandBuffer _command_buffers[max_command_buffers];
		VkFence _fences[max_command_buffers];
		std::size_t _command_index{ 0 };

		std::shared_ptr<environment_vulkan> _environment;
	};
}
