#pragma once 

#include "math.hpp"
#include "vec3.hpp"

#include <cstddef>
#include <cstring>

namespace rb {
    template<typename T>
    struct mat4 {
        static constexpr mat4<T> identity() noexcept {
            return {
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
            };
        }

        static constexpr mat4<T> translation(const vec3<T>& position) {
            return {
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                position.x, position.y, position.z, 1
            };
        }

        static constexpr mat4<T> scaling(const vec3<T>& scale) {
            return {
                scale.x, 0, 0, 0,
                0, scale.y, 0, 0,
                0, 0, scale.z, 0,
                0, 0, 0, 1
            };
        }

        static mat4<T> rotation_x(T radians) {
            const auto c = std::cos(radians);
            const auto s = std::sin(radians);

            return {
                1, 0, 0, 0,
                0, c, s, 0,
                0, -s, c, 0,
                0, 0, 0, 1
            };
        }

        static mat4<T> rotation_y(T radians) {
            const auto c = std::cos(radians);
            const auto s = std::sin(radians);

            return {
                c, 0, -s, 0,
                0, 1, 0, 0,
                s, 0, c, 0,
                0, 0, 0, 1
            };
        }

        static mat4<T> rotation_z(T radians) {
            const auto c = std::cos(radians);
            const auto s = std::sin(radians);

            return {
                c, s, 0, 0,
                -s, c, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
            };
        }

        static mat4<T> rotation(const vec3<T>& radians) {
            return rotation_y(radians.y) * rotation_x(radians.x) * rotation_z(radians.z);
        }

        const T& operator[](std::size_t index) const {
            return values[index];
        }

        T& operator[](std::size_t index) {
            return values[index];
        }

        T values[16];
    };

    template<typename T>
    bool operator==(const mat4<T>& a, const mat4<T>& b) {
        return std::memcmp(&a, &b, sizeof(mat4<T>)) == 0;
    }

    template<typename T>
    bool operator!=(const mat4<T>& a, const mat4<T>& b) {
        return std::memcmp(&a, &b, sizeof(mat4<T>)) != 0;
    }

    template<typename T>
    constexpr mat4<T> operator*(const mat4<T>& a, const mat4<T>& b) {
        return {
            a[0] * b[0] + a[4] * b[1] + a[8] * b[2] + a[12] * b[3],
            a[1] * b[0] + a[5] * b[1] + a[9] * b[2] + a[13] * b[3],
            a[2] * b[0] + a[6] * b[1] + a[10] * b[2] + a[14] * b[3],
            a[3] * b[0] + a[7] * b[1] + a[11] * b[2] + a[15] * b[3],

            a[0] * b[4] + a[4] * b[5] + a[8] * b[6] + a[12] * b[7],
            a[1] * b[4] + a[5] * b[5] + a[9] * b[6] + a[13] * b[7],
            a[2] * b[4] + a[6] * b[5] + a[10] * b[6] + a[14] * b[7],
            a[3] * b[4] + a[7] * b[5] + a[11] * b[6] + a[15] * b[7],

            a[0] * b[8] + a[4] * b[9] + a[8] * b[10] + a[12] * b[11],
            a[1] * b[8] + a[5] * b[9] + a[9] * b[10] + a[13] * b[11],
            a[2] * b[8] + a[6] * b[9] + a[10] * b[10] + a[14] * b[11],
            a[3] * b[8] + a[7] * b[9] + a[11] * b[10] + a[15] * b[11],

            a[0] * b[12] + a[4] * b[13] + a[8] * b[14] + a[12] * b[15],
            a[1] * b[12] + a[5] * b[13] + a[9] * b[14] + a[13] * b[15],
            a[2] * b[12] + a[6] * b[13] + a[10] * b[14] + a[14] * b[15],
            a[3] * b[12] + a[7] * b[13] + a[11] * b[14] + a[15] * b[15]
        };
    }

    template<typename T>
    constexpr mat4<T> invert(const mat4<T>& mat) {
        const mat4<T> out = {
            mat[5] * mat[10] * mat[15] -
            mat[5] * mat[11] * mat[14] -
            mat[9] * mat[6] * mat[15] +
            mat[9] * mat[7] * mat[14] +
            mat[13] * mat[6] * mat[11] -
            mat[13] * mat[7] * mat[10],

            -mat[1] * mat[10] * mat[15] +
            mat[1] * mat[11] * mat[14] +
            mat[9] * mat[2] * mat[15] -
            mat[9] * mat[3] * mat[14] -
            mat[13] * mat[2] * mat[11] +
            mat[13] * mat[3] * mat[10],

            mat[1] * mat[6] * mat[15] -
            mat[1] * mat[7] * mat[14] -
            mat[5] * mat[2] * mat[15] +
            mat[5] * mat[3] * mat[14] +
            mat[13] * mat[2] * mat[7] -
            mat[13] * mat[3] * mat[6],

            -mat[1] * mat[6] * mat[11] +
            mat[1] * mat[7] * mat[10] +
            mat[5] * mat[2] * mat[11] -
            mat[5] * mat[3] * mat[10] -
            mat[9] * mat[2] * mat[7] +
            mat[9] * mat[3] * mat[6],

            -mat[4] * mat[10] * mat[15] +
            mat[4] * mat[11] * mat[14] +
            mat[8] * mat[6] * mat[15] -
            mat[8] * mat[7] * mat[14] -
            mat[12] * mat[6] * mat[11] +
            mat[12] * mat[7] * mat[10],

            mat[0] * mat[10] * mat[15] -
            mat[0] * mat[11] * mat[14] -
            mat[8] * mat[2] * mat[15] +
            mat[8] * mat[3] * mat[14] +
            mat[12] * mat[2] * mat[11] -
            mat[12] * mat[3] * mat[10],

            -mat[0] * mat[6] * mat[15] +
            mat[0] * mat[7] * mat[14] +
            mat[4] * mat[2] * mat[15] -
            mat[4] * mat[3] * mat[14] -
            mat[12] * mat[2] * mat[7] +
            mat[12] * mat[3] * mat[6],

            mat[0] * mat[6] * mat[11] -
            mat[0] * mat[7] * mat[10] -
            mat[4] * mat[2] * mat[11] +
            mat[4] * mat[3] * mat[10] +
            mat[8] * mat[2] * mat[7] -
            mat[8] * mat[3] * mat[6],

            mat[4] * mat[9] * mat[15] -
            mat[4] * mat[11] * mat[13] -
            mat[8] * mat[5] * mat[15] +
            mat[8] * mat[7] * mat[13] +
            mat[12] * mat[5] * mat[11] -
            mat[12] * mat[7] * mat[9],

            -mat[0] * mat[9] * mat[15] +
            mat[0] * mat[11] * mat[13] +
            mat[8] * mat[1] * mat[15] -
            mat[8] * mat[3] * mat[13] -
            mat[12] * mat[1] * mat[11] +
            mat[12] * mat[3] * mat[9],

            mat[0] * mat[5] * mat[15] -
            mat[0] * mat[7] * mat[13] -
            mat[4] * mat[1] * mat[15] +
            mat[4] * mat[3] * mat[13] +
            mat[12] * mat[1] * mat[7] -
            mat[12] * mat[3] * mat[5],

            -mat[0] * mat[5] * mat[11] +
            mat[0] * mat[7] * mat[9] +
            mat[4] * mat[1] * mat[11] -
            mat[4] * mat[3] * mat[9] -
            mat[8] * mat[1] * mat[7] +
            mat[8] * mat[3] * mat[5],

            -mat[4] * mat[9] * mat[14] +
            mat[4] * mat[10] * mat[13] +
            mat[8] * mat[5] * mat[14] -
            mat[8] * mat[6] * mat[13] -
            mat[12] * mat[5] * mat[10] +
            mat[12] * mat[6] * mat[9],

            mat[0] * mat[9] * mat[14] -
            mat[0] * mat[10] * mat[13] -
            mat[8] * mat[1] * mat[14] +
            mat[8] * mat[2] * mat[13] +
            mat[12] * mat[1] * mat[10] -
            mat[12] * mat[2] * mat[9],

            -mat[0] * mat[5] * mat[14] +
            mat[0] * mat[6] * mat[13] +
            mat[4] * mat[1] * mat[14] -
            mat[4] * mat[2] * mat[13] -
            mat[12] * mat[1] * mat[6] +
            mat[12] * mat[2] * mat[5],

            mat[0] * mat[5] * mat[10] -
            mat[0] * mat[6] * mat[9] -
            mat[4] * mat[1] * mat[10] +
            mat[4] * mat[2] * mat[9] +
            mat[8] * mat[1] * mat[6] -
            mat[8] * mat[2] * mat[5]
        };

        const auto det = mat[0] * out[0] + mat[1] * out[4] + mat[2] * out[8] + mat[3] * out[12];
        const auto inv_det = det != 0 ? (1 / det) : 0;

        return {
            out[0] * inv_det, out[1] * inv_det, out[2] * inv_det, out[3] * inv_det,
            out[4] * inv_det, out[5] * inv_det, out[6] * inv_det, out[7] * inv_det,
            out[8] * inv_det, out[9] * inv_det, out[10] * inv_det, out[11] * inv_det,
            out[12] * inv_det, out[13] * inv_det, out[14] * inv_det, out[15] * inv_det
        };
    }


    using mat4f = mat4<float>;
}
