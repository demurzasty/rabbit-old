#pragma once 

#include "vec2.hpp"
#include "color.hpp"
#include "span.hpp"
#include "bstream.hpp"

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

        image() = default;

        image(const image&) = delete;
        image(image&&) = default;

        image& operator=(const image&) = delete;
        image& operator=(image&&) = default;

        operator bool() const;

        span<const color> pixels() const;

        const vec2u& size() const;

        std::size_t stride() const;

    private:
        image(const color* pixels, const vec2u& size);

    private:
        std::vector<color> _pixels;
        vec2u _size{ 0, 0 };
    };
}
