#pragma once 

#include "config.hpp"
#include "span.hpp"
#include "vec4.hpp"
#include "mat4.hpp"
#include "color.hpp"
#include "topology.hpp"
#include "blend_state.hpp"
#include "vertex.hpp"
#include "texture.hpp"
#include "window.hpp"
#include "buffer.hpp"
#include "shader.hpp"
#include "container.hpp"

#include <memory>

namespace rb {
    class graphics_device {
    public:
		/**
		 * @brief Install graphics device implementation to dependency container.
		 */
        static std::shared_ptr<graphics_device> resolve(container& container);

        /**
         * @brief Default virtual constructor.
         */
        virtual ~graphics_device() = default;

        /**
         * @brief Creates new texture using texture description.
         * 
         * @return Newly created texture.
         */
        virtual std::shared_ptr<texture> make_texture(const texture_desc& texture_desc) = 0;

        /**
         * @brief Creates new buffer using buffer description.
         *
         * @return Newly created buffer.
         */
        virtual std::shared_ptr<buffer> make_buffer(const buffer_desc& buffer_desc) = 0;

        /**
         * @brief Creates new shader using shader description.
         *
         * @return Newly created shader.
         */
        virtual std::shared_ptr<shader> make_shader(const shader_desc& shader_desc) = 0;


        /**
         * @brief Clears the viewport to a specified color.
         *        It's also clear depth buffer if depth testing is enabled!
         * 
         * @param color Set this color value in the buffer.
         */
        virtual void clear(const color& color) = 0;

        /**
         * @brief Swap buffers.
         */
        virtual void present() = 0;

        /**
         * @brief Sets a new blend state for a graphics device.
         *
         * @param blend_state A new blend state for the device.
         */
        virtual void set_blend_state(const blend_state& blend_state) = 0;

        /**
         * @brief Enables or disables depth testing.
         */
        virtual void set_depth_test(bool depth_test) = 0;

        /**
         * @brief Sets a new projection matrix for a graphics device.
         * 
         * @param projection A new projection matrix for the device.
         */
        virtual void set_projection_matrix(const mat4f& projection) = 0;

        /**
         * @brief Sets a new view matrix for a graphics device.
         *
         * @param projection A new view matrix for the device.
         */
        virtual void set_view_matrix(const mat4f& view) = 0;

        /**
         * @brief Sets a new world matrix for a graphics device.
         *
         * @param projection A new world matrix for the device.
         */
        virtual void set_world_matrix(const mat4f& world) = 0;

        /**
         * @brief Sets a new backbuffer size for a graphics device.
         * 
         * @param size A new backbuffer size for the device.
         */
        virtual void set_backbuffer_size(const vec2i& size) = 0;

        /**
         * @brief Returns backbuffer size.
         * 
         * @returns Backbuffer size.
         */
        virtual vec2i backbuffer_size() const = 0;

        /**
         * @brief Sets a new clip rect of the viewport using scissor test.
         * 
         * @param clip_rect Area to be clipped.
         */
        virtual void set_clip_rect(const vec4i& clip_rect) = 0;

        /**
         * @brief Sets a new offscreen render target.
         *        If no render target is provided, screen render target will be used.
         *
         * @param render_target New render target for the device.
         */
        virtual void set_render_target(const std::shared_ptr<texture>& render_target) = 0;

        /**
         * @brief Bind a buffer object to an indexed buffer target.
         */
        virtual void bind_buffer_base(const std::shared_ptr<buffer>& buffer, std::size_t binding_index) = 0;

        /**
         * @brief Draws a sequence of non-indexed geometric primitives of the specified type.
         * 
         * @param topology Describes the type of primitive to draw.
         * @param vertices Input data to draw from.
         */
        virtual void draw(topology topology, const span<const vertex>& vertices) = 0;

        virtual void draw(topology topology, std::shared_ptr<buffer> vertex_buffer) = 0;

        /**
         * @brief Draws a sequence of indexed geometric primitives of the specified type.
         *
         * @param topology Describes the type of primitive to draw.
         * @param vertices Input data to draw from.
         * @param indices Input data to index vertices.
         */
        virtual void draw(topology topology, const span<const vertex>& vertices, const span<const std::uint32_t>& indices) = 0;

        virtual void draw(topology topology, std::shared_ptr<buffer> vertex_buffer, std::shared_ptr<buffer> index_buffer) = 0;

        /**
         * @brief Draws a textured sequence of non-indexed geometric primitives of the specified type.
         *        If texture is not provided, primitives will be drawed in solid mode.
         * 
         * @param topology Describes the type of primitive to draw.
         * @param vertices Input data to draw from.
         * @param texture Texture to be used.
         */
        virtual void draw_textured(topology topology, const span<const vertex>& vertices, const std::shared_ptr<texture>& texture) = 0;

        virtual void draw_textured(topology topology, std::shared_ptr<buffer> vertex_buffer, const std::shared_ptr<texture>& texture) = 0;

        /**
         * @brief Draws a textured sequence of indexed geometric primitives of the specified type.
         *        If texture is not provided, primitives will be drawed in solid mode.
         *
         * @param topology Describes the type of primitive to draw.
         * @param vertices Input data to draw from.
         * @param texture Texture to be used.
         * @param indices Input data to index vertices.
         */
        virtual void draw_textured(topology topology, const span<const vertex>& vertices, const span<const std::uint32_t>& indices, const std::shared_ptr<texture>& texture) = 0;

        virtual void draw_textured(topology topology, std::shared_ptr<buffer> vertex_buffer, std::shared_ptr<buffer> index_buffer, const std::shared_ptr<texture>& texture) = 0;
    };
}
