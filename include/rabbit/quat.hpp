#pragma once 

#include <cmath>

#include "mat4.hpp"
#include "vec3.hpp"
#include "math.hpp"

namespace rb {
    template<typename T>
    struct quat {
        template<typename T>
        static quat<T> from_rotation_matrix(const mat4<T>& mat) {
            const auto m11 = mat[0], m12 = mat[4], m13 = mat[8];
            const auto m21 = mat[1], m22 = mat[5], m23 = mat[9];
            const auto m31 = mat[2], m32 = mat[6], m33 = mat[10];
            const auto trace = m11 + m22 + m33;

            quat<T> result;
            if (trace > 0) {
                const auto s = static_cast<T>(0.5) / std::sqrt(trace + 1);

                result.w = static_cast<T>(0.25) / s;
                result.x = (m32 - m23) * s;
                result.y = (m13 - m31) * s;
                result.z = (m21 - m12) * s;
            } else if (m11 > m22 && m11 > m33) {
                const auto s = 2.0 * std::sqrt(1.0 + m11 - m22 - m33);

                result.w = (m32 - m23) / s;
                result.x = static_cast<T>(0.25) * s;
                result.y = (m12 + m21) / s;
                result.z = (m13 + m31) / s;
            } else if (m22 > m33) {
                const auto s = 2.0 * std::sqrt(1.0 + m22 - m11 - m33);

                result.w = (m13 - m31) / s;
                result.x = (m12 + m21) / s;
                result.y = static_cast<T>(0.25) * s;
                result.z = (m23 + m32) / s;
            } else {
                const auto s = 2.0 * std::sqrt(1.0 + m33 - m11 - m22);

                result.w = (m21 - m12) / s;
                result.x = (m13 + m31) / s;
                result.y = (m23 + m32) / s;
                result.z = static_cast<T>(0.25) * s;
            }
            return result;
        }

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