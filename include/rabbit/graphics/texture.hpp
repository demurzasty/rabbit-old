#pragma once 

#include "../math/vec2.hpp"
#include "../math/vec4.hpp"
#include "../core/span.hpp"
#include "texture_format.hpp"
#include "texture_wrap.hpp"
#include "texture_filter.hpp"

#include <cstdint>

namespace rb {
    struct texture_desc {
        span<const std::uint8_t> data;
        vec2i size = { 0, 0 };
        texture_format format = texture_format::rgba8;
        texture_filter filter = texture_filter::nearest;
        texture_wrap wrap = texture_wrap::clamp;
        bool is_render_target = false;
        bool is_mutable = false;
        bool generate_mipmaps = false;
    };

    class texture {
    public:
        /**
         * @brief Default virtual destructor.
         */
        virtual ~texture() = default;

        /**
         * @brief Update a part of the texture from an array of pixels.
         * 
         *        The @a pixel array is assumed to have the same size as
         *        the @a area rectangle and to contain proper pixel format.
         * 
         *        This function does nothing if @a pixels is empty or if the
         *        texture was not previously created with @a mutable flag.
         * 
         * @param pixels Array of pixels to copy to the texture.
         * @param area Rectangle of the pixel region contained in @a pixels.
         */
        virtual void update(const span<const std::uint8_t>& pixels, const vec4i& area) = 0;

        /**
         * @brief Returns the size of the texture.
         * 
         * @return Size in pixels.
         */
        const vec2i& size() const;

        /**
         * @brief Returns the texel size.
         * 
         * @return Texel size, i.e. { 1 / width, 1 / height }.
         */
        vec2f texel() const;

        /**
         * @brief Returns the texture format.
         * 
         * @return Texture format.
         */
        texture_format format() const;

        /**
         * @brief Returns the texture filter.
         *
         * @return Texture filter.
         */
        texture_filter filter() const;

        /**
         * @brief Returns the texture wrap.
         *
         * @return Texture wrap.
         */
        texture_wrap wrap() const;

        /**
         * @brief Tell whether the texture is render target.
         *
         * @return True if is render target.
         */
        bool is_render_target() const;

        /**
         * @brief Tell whether the texture is mutable.
         *
         * @return True if is mutable.
         */
        bool is_mutable() const;

    protected:
        /**
         * @brief Creates texture using description.
         */
        texture(const texture_desc& desc);

    private:
        vec2i _size;
        texture_format _format;
        texture_filter _filter;
        texture_wrap _wrap;
        bool _is_render_target;
        bool _is_mutable;
    };
}
