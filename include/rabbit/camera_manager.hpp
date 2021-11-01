#pragma once 

#include "entity.hpp"

namespace rb {
    class camera_manager {
    public:
        static void init();

        static void release();

    public:
        static entity main_camera;
    };
}
