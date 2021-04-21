#pragma once 

#include "mesh.hpp"
#include "material.hpp"
#include "resource_heap.hpp"
#include "texture.hpp"

#include <memory>

namespace rb {
    struct geometry {
        std::shared_ptr<mesh> mesh;
        std::shared_ptr<material> material;
        std::shared_ptr<resource_heap> resource_heap;
        std::shared_ptr<texture> albedo_map;
    };
}
