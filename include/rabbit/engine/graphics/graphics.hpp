#pragma once 

#include <rabbit/engine/graphics/viewport.hpp>
#include <rabbit/engine/graphics/environment.hpp>
#include <rabbit/engine/graphics/texture.hpp>
#include <rabbit/engine/graphics/material.hpp>
#include <rabbit/engine/graphics/mesh.hpp>

// TODO: Remove. Do not depend on runtime module.
#include <rabbit/runtime/components/transform.hpp>
#include <rabbit/runtime/components/camera.hpp>
#include <rabbit/runtime/components/geometry.hpp>
#include <rabbit/runtime/components/light.hpp>

#include <memory>

namespace rb {
	enum class graphics_backend {
		vulkan
	};

	struct graphics_limits {
		static constexpr std::size_t max_lights{ 1024 };
		static constexpr std::size_t brdf_map_size{ 512 };
		static constexpr std::size_t irradiance_map_size{ 64 };
		static constexpr std::size_t prefilter_map_size{ 128 };
		static constexpr std::size_t shadow_map_size{ 1024 };
		static constexpr std::size_t max_shadow_cascades{ 4 };
		static constexpr std::size_t ssao_image_reduction{ 4 };
	};

	class graphics_impl {
	public:
		virtual ~graphics_impl() = default;

		virtual std::shared_ptr<viewport> make_viewport(const viewport_desc& desc) = 0;

		virtual std::shared_ptr<texture> make_texture(const texture_desc& desc) = 0;

		virtual std::shared_ptr<environment> make_environment(const environment_desc& desc) = 0;

		virtual std::shared_ptr<material> make_material(const material_desc& desc) = 0;

		virtual std::shared_ptr<mesh> make_mesh(const mesh_desc& desc) = 0;

		virtual void begin() = 0;

		virtual void set_camera(const mat4f& projection, const mat4f& view, const mat4f& world, const std::shared_ptr<environment>& environment) = 0;

		virtual void begin_depth_pass(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void draw_depth(const std::shared_ptr<viewport>& viewport, const mat4f& world, const std::shared_ptr<mesh>& mesh, std::size_t mesh_lod_index) = 0;

		virtual void end_depth_pass(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void begin_shadow_pass(const transform& transform, const light& light, const directional_light& directional_light, std::size_t cascade) = 0;

		virtual void draw_shadow(const mat4f& world, const geometry& geometry, std::size_t cascade) = 0;

		virtual void end_shadow_pass() = 0;

		virtual void begin_light_pass(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void add_point_light(const std::shared_ptr<viewport>& viewport, const transform& transform, const light& light, const point_light& point_light) = 0;

		virtual void add_directional_light(const std::shared_ptr<viewport>& viewport, const transform& transform, const light& light, const directional_light& directional_light, bool use_shadow) = 0;

		virtual void end_light_pass(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void begin_forward_pass(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void draw_skybox(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void draw_forward(const std::shared_ptr<viewport>& viewport, const mat4f& world, const std::shared_ptr<mesh>& mesh, const std::shared_ptr<material>& material, std::size_t mesh_lod_index) = 0;

		virtual void end_forward_pass(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void pre_draw_ssao(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void begin_fill_pass(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void draw_fill(const std::shared_ptr<viewport>& viewport, const transform& transform, const geometry& geometry) = 0;

		virtual void end_fill_pass(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void begin_postprocess_pass(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void next_postprocess_pass(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void draw_ssao(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void draw_fxaa(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void draw_blur(const std::shared_ptr<viewport>& viewport, int strength) = 0;

		virtual void draw_sharpen(const std::shared_ptr<viewport>& viewport, float strength) = 0;

		virtual void draw_motion_blur(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void draw_outline(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void end_postprocess_pass(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void begin_canvas_pass() = 0;

		virtual void draw_canvas_textured(const span<const canvas_vertex>& vertices, const std::shared_ptr<texture>& texture) = 0;

		virtual void end_canvas_pass() = 0;

		virtual void present(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void end() = 0;

		virtual void swap_buffers() = 0;

		virtual void flush() = 0;
	};

	class graphics {
	public:
		static void init();

		static void release();

		static std::shared_ptr<viewport> make_viewport(const viewport_desc& desc);

		static std::shared_ptr<texture> make_texture(const texture_desc& desc);

		static std::shared_ptr<environment> make_environment(const environment_desc& desc);

		static std::shared_ptr<material> make_material(const material_desc& desc);

		static std::shared_ptr<mesh> make_mesh(const mesh_desc& desc);

		static void begin();

		static void set_camera(const mat4f& projection, const mat4f& view, const mat4f& world, const std::shared_ptr<environment>& environment);

		static void begin_depth_pass(const std::shared_ptr<viewport>& viewport);

		static void draw_depth(const std::shared_ptr<viewport>& viewport, const mat4f& world, const std::shared_ptr<mesh>& mesh, std::size_t mesh_lod_index);

		static void end_depth_pass(const std::shared_ptr<viewport>& viewport);

		static void begin_shadow_pass(const transform& transform, const light& light, const directional_light& directional_light, std::size_t cascade);

		static void draw_shadow(const mat4f& world, const geometry& geometry, std::size_t cascade);

		static void end_shadow_pass();

		static void begin_light_pass(const std::shared_ptr<viewport>& viewport);

		static void add_point_light(const std::shared_ptr<viewport>& viewport, const transform& transform, const light& light, const point_light& point_light);

		static void add_directional_light(const std::shared_ptr<viewport>& viewport, const transform& transform, const light& light, const directional_light& directional_light, bool use_shadow);

		static void end_light_pass(const std::shared_ptr<viewport>& viewport);

		static void begin_forward_pass(const std::shared_ptr<viewport>& viewport);

		static void draw_skybox(const std::shared_ptr<viewport>& viewport);

		static void draw_forward(const std::shared_ptr<viewport>& viewport, const mat4f& world, const std::shared_ptr<mesh>& mesh, const std::shared_ptr<material>& material, std::size_t mesh_lod_index);

		static void end_forward_pass(const std::shared_ptr<viewport>& viewport);

		static void pre_draw_ssao(const std::shared_ptr<viewport>& viewport);

		static void begin_fill_pass(const std::shared_ptr<viewport>& viewport);

		static void draw_fill(const std::shared_ptr<viewport>& viewport, const transform& transform, const geometry& geometry);

		static void end_fill_pass(const std::shared_ptr<viewport>& viewport);

		static void begin_postprocess_pass(const std::shared_ptr<viewport>& viewport);

		static void next_postprocess_pass(const std::shared_ptr<viewport>& viewport);

		static void draw_ssao(const std::shared_ptr<viewport>& viewport);

		static void draw_fxaa(const std::shared_ptr<viewport>& viewport);

		static void draw_blur(const std::shared_ptr<viewport>& viewport, int strength);

		static void draw_sharpen(const std::shared_ptr<viewport>& viewport, float strength);

		static void draw_motion_blur(const std::shared_ptr<viewport>& viewport);

		static void draw_outline(const std::shared_ptr<viewport>& viewport);

		static void end_postprocess_pass(const std::shared_ptr<viewport>& viewport);

		static void begin_canvas_pass();

		static void draw_canvas_textured(const span<const canvas_vertex>& vertices, const std::shared_ptr<texture>& texture);

		static void end_canvas_pass();

		static void present(const std::shared_ptr<viewport>& viewport);

		static void end();
		
		static void swap_buffers();

		static void flush();

	private:
		static std::shared_ptr<graphics_impl> _impl;
	};
}
