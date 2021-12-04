#pragma once 

#include "../math/vec2.hpp"

#include <string>

namespace rb {
    struct app_settings {
        std::string data_directory;
    };

    struct time_settings {
        float fixed_time_step{ 1.0f / 60.0f };
    };

    struct window_settings {
        std::string title{ "RabBit" };
        vec2u size{ 1440, 810 };
        bool fullscreen{ false };
    };

    struct graphics_settings {
        bool vsync{ true };
    };

    struct settings {
        static app_settings app;
        static time_settings time;
        static window_settings window;
        static graphics_settings graphics;
    };
}
