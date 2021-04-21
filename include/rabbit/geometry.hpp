#pragma once 

#include "mesh.hpp"
#include "material.hpp"

#include <memory>

namespace rb {
    struct geometry {
        std::shared_ptr<mesh> mesh;
        std::shared_ptr<material> material;
    };
}
