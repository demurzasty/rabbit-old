#pragma once 

#include "../math/vec3.hpp"
#include "texture.hpp"

#include <memory>

namespace rb {
    struct material {
        vec3f diffuse{ 1.0f, 1.0f, 1.0f };
        float roughness{ 0.8f };
        float metallic{ 0.0f };
        std::shared_ptr<texture> diffuse_map{ nullptr };
        std::shared_ptr<texture> normal_map{ nullptr };
        std::shared_ptr<texture> roughness_map{ nullptr };
        std::shared_ptr<texture> metallic_map{ nullptr };
        std::shared_ptr<texture> emissive_map{ nullptr };
    };
}
