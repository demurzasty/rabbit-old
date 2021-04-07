#pragma once 

#include "mesh.hpp"

#include <memory>

namespace rb {
    struct geometry {
        std::shared_ptr<mesh> mesh;
    };
}
