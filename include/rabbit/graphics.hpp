#pragma once 

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

	class graphics_impl {
	public:
		virtual ~graphics_impl() = default;

		virtual std::shared_ptr<texture> make_texture(const texture_desc& desc) = 0;

		virtual std::shared_ptr<environment> make_environment(const environment_desc& desc) = 0;

		virtual std::shared_ptr<material> make_material(const material_desc& desc) = 0;

		virtual std::shared_ptr<mesh> make_mesh(const mesh_desc& desc) = 0;

		virtual void begin() = 0;

		virtual void end() = 0;

		virtual void set_camera(const transform& transform, const camera& camera) = 0;

		virtual void add_directional_light(transform& transform, light& light, directional_light& directional_light) = 0;

		virtual void add_point_light(transform& transform, light& light, point_light& point_light) = 0;

		virtual void begin_shadow_pass() = 0;

		virtual void draw_shadow(const transform& transform, const geometry& geometry) = 0;

		virtual void end_shadow_pass() = 0;

		virtual void begin_render_pass() = 0;

		virtual void draw_geometry(const transform& transform, const geometry& geometry) = 0;

		virtual void end_render_pass() = 0;

		virtual void draw_skybox(const std::shared_ptr<environment>& environment) = 0;

		virtual void present() = 0;

		virtual void flush() = 0;
	};

	class graphics {
	public:
		static void init();

		static void release();

		static std::shared_ptr<texture> make_texture(const texture_desc& desc);

		static std::shared_ptr<environment> make_environment(const environment_desc& desc);

		static std::shared_ptr<material> make_material(const material_desc& desc);

		static std::shared_ptr<mesh> make_mesh(const mesh_desc& desc);

		static void begin();

		static void set_camera(const transform& transform, const camera& camera);

		static void add_directional_light(transform& transform, light& light, directional_light& directional_light);

		static void add_point_light(transform& transform, light& light, point_light& point_light);

		static void begin_shadow_pass();

		static void draw_shadow(const transform& transform, const geometry& geometry);

		static void end_shadow_pass();

		static void begin_render_pass();

		static void draw_geometry(const transform& transform, const geometry& geometry);

		static void draw_skybox(const std::shared_ptr<environment>& environment);

		static void end_render_pass();

		static void end();
		
		static void present();

		static void flush();

	private:
		static std::shared_ptr<graphics_impl> _impl;
	};
}
