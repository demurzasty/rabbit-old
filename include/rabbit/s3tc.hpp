#pragma once 

#include "image.hpp"

#include <vector>
#include <cstddef>

namespace rb {
    class s3tc {
    public:
        static void bc1(const void* uncompressed_pixels, std::size_t uncompressed_size, std::size_t stride, void* compressed_pixels);

        static std::vector<std::uint8_t> bc1(const image& image);
    };
}