#pragma once 

#include "../math/math.hpp"
#include "../graphics/color.hpp"
#include "../core/config.hpp"

namespace rb {
    namespace detail {
        struct RB_NOVTABLE base_light {
            color color{ color::white() };
            float intensity{ 1.0f };
        };
    }

    struct point_light : public detail::base_light {
        float radius{ 1.0f };
    };

    struct spot_light : public detail::base_light {
        float range{ 1.0f };
        float angle{ deg2rad(45.0f) };
    };

    struct directional_light : public detail::base_light {
    };
}
