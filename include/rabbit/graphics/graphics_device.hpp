#pragma once

#include "../core/config.hpp"
#include "../core/di.hpp"
#include "../math/vec4.hpp"
#include "buffer.hpp"
#include "mesh.hpp"
#include "material.hpp"
#include "shader.hpp"
#include "shader_data.hpp"
#include "texture.hpp"

#include <memory>

namespace rb {
    enum class builtin_shader {
        forward
    };

    class graphics_device {
    public:
        /**
		 * @brief Install window implementation to dependency container.
		 */
		static void install(installer<graphics_device>& installer);

		/**
		 * @brief Default virtual destructor.
		 */
        virtual ~graphics_device() = default;

        /**
         * @brief Creates new buffer using buffer description.
         *
         * @return Newly created buffer.
         */
        RB_NODISCARD virtual std::shared_ptr<buffer> make_buffer(const buffer_desc& desc) = 0;

        /**
         * @brief Creates new mesh using mesh description.
         *
         * @return Newly created mesh.
         */
        RB_NODISCARD virtual std::shared_ptr<mesh> make_mesh(const mesh_desc& desc) = 0;

        /**
         * @brief Creates new material using material description.
         *
         * @return Newly created material.
         */
        RB_NODISCARD virtual std::shared_ptr<material> make_material(const material_desc& desc) = 0;

        /**
         * @brief Creates new shader using shader description.
         *
         * @return Newly created shader.
         */
        RB_NODISCARD virtual std::shared_ptr<shader> make_shader(const shader_desc& desc) = 0;

        /**
         * @brief Creates new shader builtin shader.
         *
         * @return Newly created shader.
         */
        RB_NODISCARD virtual std::shared_ptr<shader> make_shader(builtin_shader builtin_shader) = 0;

        /**
         * @brief Creates new shader data using shader data description.
         *
         * @return Newly created shader data.
         */
        RB_NODISCARD virtual std::shared_ptr<shader_data> make_shader_data(const shader_data_desc& desc) = 0;

        /**
         * @brief Creates new texture using texture description.
         *
         * @return Newly created texture.
         */
        RB_NODISCARD virtual std::shared_ptr<texture> make_texture(const texture_desc& desc) = 0;

        virtual void begin() = 0;

        virtual void end() = 0;

        virtual void begin_render_pass() = 0;

        virtual void end_render_pass() = 0;

        virtual void update_buffer(const std::shared_ptr<buffer>& buffer, const void* data, std::size_t offset, std::size_t size) = 0;

        virtual void set_viewport(const vec4i& viewport) = 0;

        virtual void draw(const std::shared_ptr<mesh>& mesh, const std::shared_ptr<shader>& shader, const std::shared_ptr<shader_data>& shader_data) = 0;

        /**
         * @brief Swap buffers.
         */
        virtual void present() = 0;
    };
}
