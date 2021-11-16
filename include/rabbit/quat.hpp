#pragma once 

#include <cmath>

#include "mat4.hpp"
#include "vec3.hpp"
#include "math.hpp"

namespace rb {
    template<typename T>
    struct quat {
        T w, x, y, z;
    };

    template<typename T>
    vec3<T> euler_angles(const quat<T>& quat) {
        const auto qz = quat.z;
        const auto qx = quat.x;
        const auto qy = quat.y;
        const auto qw = quat.w;

        const auto sqw = qw * qw;
        const auto sqz = qz * qz;
        const auto sqx = qx * qx;
        const auto sqy = qy * qy;

        const auto z_axis_y = qy * qz - qx * qw;
        const auto limit = static_cast<T>(.4999999);

        vec3<T> result;
        if (z_axis_y < -limit) {
            result.y = 2 * std::atan2(qy, qw);
            result.x = pi<T>() / 2;
            result.z = 0;
        } else if (z_axis_y > limit) {
            result.y = 2 * std::atan2(qy, qw);
            result.x = -pi<T>() / 2;
            result.z = 0;
        } else {
            result.z = std::atan2(2 * (qx * qy + qz * qw), (-sqz - sqx + sqy + sqw));
            result.x = std::asin(-2 * (qz * qy - qx * qw));
            result.y = std::atan2(2 * (qz * qx + qy * qw), (sqz - sqx - sqy + sqw));
        }
        return result;
    }

    using quatf = quat<float>;
}