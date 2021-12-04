#pragma once 

#include "math.hpp"
#include "vec3.hpp"

#include <cstring>

namespace rb {
    template<typename T>
    struct mat3 {
        static constexpr mat3<T> identity() noexcept {
            return {
                1, 0, 0,
                0, 1, 0,
                0, 0, 1
            };
        }


        static mat3<T> rotation_x(T radians) {
            const auto c = rb::cos(radians);
            const auto s = rb::sin(radians);

            return {
                1, 0, 0,
                0, c, s,
                0, -s, c
            };
        }

        static mat3<T> rotation_y(T radians) {
            const auto c = rb::cos(radians);
            const auto s = rb::sin(radians);

            return {
                c, 0, -s,
                0, 1, 0,
                s, 0, c
            };
        }

        static mat3<T> rotation_z(T radians) {
            const auto c = rb::cos(radians);
            const auto s = rb::sin(radians);

            return {
                c, s, 0,
                -s, c, 0,
                0, 0, 1
            };
        }

        static mat3<T> rotation(const vec3<T>& radians) {
            return rotation_y(radians.y) * rotation_x(radians.x) * rotation_z(radians.z);
        }

        const T& operator[](std::size_t index) const {
            return values[index];
        }

        T& operator[](std::size_t index) {
            return values[index];
        }

        T values[9];
    };


    template<typename T>
    bool operator==(const mat3<T>& a, const mat3<T>& b) {
        return std::memcmp(&a, &b, sizeof(mat3<T>)) == 0;
    }

    template<typename T>
    bool operator!=(const mat3<T>& a, const mat3<T>& b) {
        return std::memcmp(&a, &b, sizeof(mat3<T>)) != 0;
    }

    template<typename T>
    constexpr mat3<T> operator*(const mat3<T>& a, const mat3<T>& b) {
        return {
            a[0] * b[0] + a[3] * b[1] + a[6] * b[2],
            a[1] * b[0] + a[4] * b[1] + a[7] * b[2],
            a[2] * b[0] + a[5] * b[1] + a[8] * b[2],

            a[0] * b[3] + a[3] * b[4] + a[6] * b[5],
            a[1] * b[3] + a[4] * b[4] + a[7] * b[5],
            a[2] * b[3] + a[5] * b[4] + a[8] * b[5],

            a[0] * b[6] + a[3] * b[7] + a[6] * b[8],
            a[1] * b[6] + a[4] * b[7] + a[7] * b[8],
            a[2] * b[6] + a[5] * b[7] + a[8] * b[8]
        };
    }

    using mat3f = mat3<float>;
}