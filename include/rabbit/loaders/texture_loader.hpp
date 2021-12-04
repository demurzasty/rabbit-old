#pragma once 

#include "../core/loader.hpp"
#include "../graphics/texture.hpp"

#include <vector>
#include <cstdint>

namespace rb {
    struct texture_data {
        std::vector<std::uint8_t> data;
        vec2u size{ 0, 0 };
        texture_format format{ texture_format::rgba8 };
        texture_filter filter{ texture_filter::linear };
        texture_wrap wrap{ texture_wrap::repeat };
        std::uint32_t mipmaps{ 0 };
    };

    class texture_loader : public loader {
    public:
        std::shared_ptr<void> load(const std::string_view& filename) override;
    };
}
