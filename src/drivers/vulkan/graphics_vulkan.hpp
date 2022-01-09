#pragma once 

#include "graphics_base_vulkan.hpp"

#include <vector>

namespace rb {
	class graphics_vulkan : public graphics_base_vulkan {
		struct alignas(16) main_data {
			mat4f proj;
			mat4f view;
			mat4f proj_view;
			mat4f inv_proj_view;
			vec4f camera_position;
			vec4f camera_frustum;
			std::uint32_t instance_count;
		};

		struct alignas(16) world_data {
			mat4f transform;
			bspheref bsphere;
		};

		static constexpr auto max_world_size = 8192u;
		static constexpr auto max_vertex_size_in_bytes = 0x8000000u;
		static constexpr auto max_index_size_in_bytes = 0x8000000u;

	public:
		graphics_vulkan();

		virtual ~graphics_vulkan();

		std::shared_ptr<mesh> make_mesh(const mesh_desc& desc) override;

		void set_camera(const mat4f& proj, const mat4f& view, const mat4f& world) override;

		void render(const mat4f& world, const std::shared_ptr<mesh>& mesh, const std::shared_ptr<material>& material) override;
	
		void present() override;

	private:
		void _create_main_buffer();

		void _create_world_buffer();

		void _create_vertex_buffer();

		void _create_index_buffer();

		void _create_draw_buffer();

		void _create_forward_pipeline();

		void _create_culling_pipeline();

	private:
		std::uint32_t _instance_index{ 0 };

		VkBuffer _main_buffer;
		VmaAllocation _main_buffer_allocation;
		main_data _main_buffer_staging;

		VkBuffer _world_buffer;
		VmaAllocation _world_buffer_allocation;
		std::vector<world_data> _world_buffer_staging;

		VkBuffer _vertex_buffer;
		VmaAllocation _vertex_buffer_allocation;
		std::uint32_t _vertex_buffer_offset{ 0 };

		VkBuffer _index_buffer;
		VmaAllocation _index_buffer_allocation;
		std::uint32_t _index_buffer_offset{ 0 };

		VkBuffer _draw_buffer;
		VmaAllocation _draw_buffer_allocation;
		VkBuffer _draw_output_buffer;
		VmaAllocation _draw_output_buffer_allocation;
		std::vector<VkDrawIndexedIndirectCommand> _draw_buffer_staging;

		VkDescriptorSetLayout _forward_descriptor_set_layout;
		VkDescriptorPool _forward_descriptor_pool;
		VkDescriptorSet _forward_descriptor_set;
		VkPipelineLayout _forward_pipeline_layout;
		VkPipeline _forward_pipeline;

		VkDescriptorSetLayout _culling_descriptor_set_layout;
		VkDescriptorPool _culling_descriptor_pool;
		VkDescriptorSet _culling_descriptor_set;
		VkPipelineLayout _culling_pipeline_layout;
		VkPipeline _culling_pipeline;
	};
}
