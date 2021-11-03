#pragma once 

#include "vec2.hpp"

#include <vector>
#include <memory>
#include <string>
#include <cstdint>
#include <optional>

namespace rb {
    class image {
    public:
        static image load_from_file(const std::string& filename);

        image() = default;

        image(const image&) = delete;
        image(image&&) = default;

        image& operator=(const image&) = delete;
        image& operator=(image&&) = default;

        operator bool() const;

        const std::vector<std::uint8_t>& pixels() const;

        const vec2u& size() const;

        std::size_t stride() const;

    private:
        image(const std::uint8_t* pixels, const vec2u& size);

    private:
        std::vector<std::uint8_t> _pixels;
        vec2u _size{ 0, 0 };
    };
}
