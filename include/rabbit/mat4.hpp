#pragma once 

#include "vec3.hpp"

#include <cmath>

namespace rb {
	template<typename T>
	struct mat4 {
        static constexpr mat4<T> identity() {
            return {
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
            };
        }

        static mat4<T> perspective(T fovy, T aspect, T near, T far) {
            const auto r = fovy / 2;
            const auto delta = near - far;
            const auto s = std::sin(r);

            if (delta == 0 || s == 0 || aspect == 0) {
                return mat4<T>::identity();
            }

            const auto cotangent = std::cos(r) / s;

            return {
                cotangent / aspect, 0, 0, 0,
                0, cotangent, 0, 0,
                0, 0, (near + far) / delta, -1,
                0, 0, (2 * near * far) / delta, 0
            };
        }

        static constexpr mat4<T> orthographic(T left, T right, T bottom, T top, T near, T far) {
            const auto tx = -((right + left) / (right - left));
            const auto ty = -((top + bottom) / (top - bottom));
            const auto tz = -((far + near) / (far - near));

            return {
                2 / (right - left), 0, 0, 0,
                0, 2 / (top - bottom), 0, 0,
                0, 0, -2 / (far - near), 0,
                tx, ty, tz, 1
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

        static constexpr mat4<T> scaling(const vec3<T>& scaling) {
            return {
                scaling.x, 0, 0, 0,
                0, scaling.y, 0, 0,
                0, 0, scaling.z, 0,
                0, 0, 0, 1
            };
        }

        static mat4<T> look_at(const vec3<T>& eye, const vec3<T>& target, const vec3<T>& up) {
            vec3<T> f = normalize(target - eye);
            vec3<T> s = normalize(cross(f, up));
            vec3<T> u = cross(s, f);

            return {
                s.x, u.x, -f.x, 0,
                s.y, u.y, -f.y, 0,
                s.z, u.z, -f.z, 0,
                -dot(s, eye), -dot(u, eye), dot(f, eye), 1.0f
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

        constexpr const T& operator[](std::size_t index) const {
            return values[index];
        }

        constexpr T& operator[](std::size_t index) {
            return values[index];
        }

		T values[16];
	};

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

    template<typename T>
    constexpr vec3<T> transform_normal(const mat4<T>& a, const vec3<T>& b) {
        return {
            b.x * a[0] + b.y * a[4] + b.z * a[8],
            b.x * a[1] + b.y * a[5] + b.z * a[9],
            b.x * a[2] + b.y * a[6] + b.z * a[10]
        };
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

	using mat4f = mat4<float>;
}
