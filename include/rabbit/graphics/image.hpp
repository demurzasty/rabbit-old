#pragma once 

#include "color.hpp"
#include "../core/span.hpp"
#include "../core/bstream.hpp"
#include "../math/vec2.hpp"

#include <vector>
#include <memory>
#include <string>
#include <cstdint>
#include <optional>

namespace rb {
    class image {
    public:
        static image load_from_file(const std::string& filename);

        static image load_from_stream(ibstream& stream);

        static image resize(const image& image, const vec2u& new_size);

        image() = default;

        image(const image&) = delete;
        image(image&&) = default;

        image& operator=(const image&) = delete;
        image& operator=(image&&) = default;

        operator bool() const;

        span<const color> pixels() const;

        const vec2u& size() const;

        std::size_t stride() const;

        bool is_power_of_two() const;

    private:
        image(const color* pixels, const vec2u& size);

    private:
        std::vector<color> _pixels;
        vec2u _size{ 0, 0 };
    };
}
