#pragma once 

#include "viewport.hpp"
#include "environment.hpp"
#include "texture.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "transform.hpp"
#include "camera.hpp"
#include "geometry.hpp"
#include "light.hpp"

#include <memory>

namespace rb {
	enum class graphics_backend {
		vulkan
	};

	struct graphics_limits {
		static constexpr std::size_t max_lights{ 16 };
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

		virtual void set_camera(const transform& transform, const camera& camera) = 0;

		virtual void begin_geometry_pass(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void draw_geometry(const std::shared_ptr<viewport>& viewport, const transform& transform, const geometry& geometry) = 0;

		virtual void end_geometry_pass(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void begin_shadow_pass(const transform& transform, const light& light, const directional_light& directional_light, std::size_t cascade) = 0;

		virtual void draw_shadow(const transform& transform, const geometry& geometry, std::size_t cascade) = 0;

		virtual void end_shadow_pass() = 0;

		virtual void begin_light_pass(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void draw_ambient(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void draw_directional_light(const std::shared_ptr<viewport>& viewport, const transform& transform, const light& light, const directional_light& directional_light) = 0;

		virtual void draw_point_light(const std::shared_ptr<viewport>& viewport, const transform& transform, const light& light, const point_light& point_light) = 0;

		virtual void draw_skybox(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void end_light_pass(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void begin_forward_pass(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void end_forward_pass(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void pre_draw_ssao(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void begin_postprocess_pass(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void next_postprocess_pass(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void draw_ssao(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void draw_fxaa(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void draw_blur(const std::shared_ptr<viewport>& viewport, int strength) = 0;

		virtual void draw_sharpen(const std::shared_ptr<viewport>& viewport, float strength) = 0;

		virtual void draw_motion_blur(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void end_postprocess_pass(const std::shared_ptr<viewport>& viewport) = 0;

		virtual void begin_immediate_pass() = 0;

		virtual void draw_immediate_color(const span<const vertex>& vertices, const color& color) = 0;

		virtual void draw_immediate_textured(const span<const vertex>& vertices, const std::shared_ptr<texture>& texture) = 0;

		virtual void end_immediate_pass() = 0;

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

		static void set_camera(const transform& transform, const camera& camera);

		static void begin_geometry_pass(const std::shared_ptr<viewport>& viewport);

		static void draw_geometry(const std::shared_ptr<viewport>& viewport, const transform& transform, const geometry& geometry);

		static void end_geometry_pass(const std::shared_ptr<viewport>& viewport);

		static void begin_shadow_pass(const transform& transform, const light& light, const directional_light& directional_light, std::size_t cascade);

		static void draw_shadow(const transform& transform, const geometry& geometry, std::size_t cascade);

		static void end_shadow_pass();

		static void begin_light_pass(const std::shared_ptr<viewport>& viewport);

		static void draw_ambient(const std::shared_ptr<viewport>& viewport);

		static void draw_directional_light(const std::shared_ptr<viewport>& viewport, const transform& transform, const light& light, const directional_light& directional_light);

		static void draw_point_light(const std::shared_ptr<viewport>& viewport, const transform& transform, const light& light, const point_light& point_light);

		static void draw_skybox(const std::shared_ptr<viewport>& viewport);

		static void end_light_pass(const std::shared_ptr<viewport>& viewport);

		static void begin_forward_pass(const std::shared_ptr<viewport>& viewport);

		static void end_forward_pass(const std::shared_ptr<viewport>& viewport);

		static void pre_draw_ssao(const std::shared_ptr<viewport>& viewport);

		static void begin_postprocess_pass(const std::shared_ptr<viewport>& viewport);

		static void next_postprocess_pass(const std::shared_ptr<viewport>& viewport);

		static void draw_ssao(const std::shared_ptr<viewport>& viewport);

		static void draw_fxaa(const std::shared_ptr<viewport>& viewport);

		static void draw_blur(const std::shared_ptr<viewport>& viewport, int strength);

		static void draw_sharpen(const std::shared_ptr<viewport>& viewport, float strength);

		static void draw_motion_blur(const std::shared_ptr<viewport>& viewport);

		static void end_postprocess_pass(const std::shared_ptr<viewport>& viewport);

		static void begin_immediate_pass();

		static void draw_immediate_color(const span<const vertex>& vertices, const color& color);

		static void draw_immediate_textured(const span<const vertex>& vertices, const std::shared_ptr<texture>& texture);

		static void end_immediate_pass();

		static void present(const std::shared_ptr<viewport>& viewport);

		static void end();
		
		static void swap_buffers();

		static void flush();

	private:
		static std::shared_ptr<graphics_impl> _impl;
	};
}
