#pragma once 

#include "viewport.hpp"
#include "environment.hpp"
#include "texture.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "shader.hpp"

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

		virtual std::shared_ptr<texture> make_texture(const texture_desc& desc) = 0;

		virtual std::shared_ptr<environment> make_environment(const environment_desc& desc) = 0;

		virtual std::shared_ptr<material> make_material(const material_desc& desc) = 0;

		virtual std::shared_ptr<mesh> make_mesh(const mesh_desc& desc) = 0;

		virtual void set_camera(const mat4f& proj, const mat4f& view, const mat4f& world) = 0;

		virtual void render(const mat4f& world, const std::shared_ptr<mesh>& mesh, const std::shared_ptr<material>& material) = 0;

		virtual void present() = 0;
	};

	class graphics {
	public:
		static void init();

		static void release();

		static std::shared_ptr<texture> make_texture(const texture_desc& desc);

		static std::shared_ptr<environment> make_environment(const environment_desc& desc);

		static std::shared_ptr<material> make_material(const material_desc& desc);

		static std::shared_ptr<mesh> make_mesh(const mesh_desc& desc);

		static void set_camera(const mat4f& proj, const mat4f& view, const mat4f& world);

		static void render(const mat4f& world, const std::shared_ptr<mesh>& mesh, const std::shared_ptr<material>& material);

		static void present();

	private:
		static std::shared_ptr<graphics_impl> _impl;
	};
}
