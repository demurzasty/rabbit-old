#pragma once 

#include "../graphics/mesh.hpp"
#include "../graphics/material.hpp"
#include "../graphics/instance.hpp"

#include <memory>

namespace rb {
    struct geometry {
        instance instance{ null };
        std::shared_ptr<mesh> mesh;
        std::shared_ptr<material> material;
    };
}
