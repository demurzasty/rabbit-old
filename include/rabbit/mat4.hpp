#pragma once 

#include "vec3.hpp"
#include "vec4.hpp"
#include "quat.hpp"

#include <cmath>
#include <limits>

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

        static mat4<T> perspective(T fovy, T aspect, T z_min, T z_far) {
            const auto r = fovy / 2;
            const auto delta = z_min - z_far;
            const auto s = std::sin(r);

            if (delta == 0 || s == 0 || aspect == 0) {
                return mat4<T>::identity();
            }

            const auto cotangent = std::cos(r) / s;

            return {
                cotangent / aspect, 0, 0, 0,
                0, cotangent, 0, 0,
                0, 0, (z_min + z_far) / delta, -1,
                0, 0, (2 * z_min * z_far) / delta, 0
            };
        }

        static constexpr mat4<T> orthographic(T left, T right, T bottom, T top, T z_near, T z_far) {
            const auto tx = -((right + left) / (right - left));
            const auto ty = -((top + bottom) / (top - bottom));
            const auto tz = -((z_far + z_near) / (z_far - z_near));

            return {
                2 / (right - left), 0, 0, 0,
                0, 2 / (top - bottom), 0, 0,
                0, 0, -2 / (z_far - z_near), 0,
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

        static constexpr mat4<T> rotation(const quat<T>& quat) {
            const auto xx = quat.x * quat.x;
            const auto xy = quat.x * quat.y;
            const auto xz = quat.x * quat.z;
            const auto xw = quat.x * quat.w;

            const auto yy = quat.y * quat.y;
            const auto yz = quat.y * quat.z;
            const auto yw = quat.y * quat.w;

            const auto zz = quat.z * quat.z;
            const auto zw = quat.z * quat.w;

            mat4<T> mat;
            mat[0] = 1 - 2 * (yy + zz);
            mat[1] = 2 * (xy + zw);
            mat[2] = 2 * (xz - yw);
            mat[3] = 0;

            mat[4] = 2 * (xy - zw);
            mat[5] = 1 - 2 * (xx + zz);
            mat[6] = 2 * (yz + xw);
            mat[7] = 0;

            mat[8] = 2 * (xz + yw);
            mat[9] = 2 * (yz - xw);
            mat[10] = 1 - 2 * (xx + yy);
            mat[11] = 0;

            mat[12] = 0;
            mat[13] = 0;
            mat[14] = 0;
            mat[15] = 1;
            return mat;
        }

        constexpr const T& operator[](std::size_t index) const {
            return values[index];
        }

        constexpr T& operator[](std::size_t index) {
            return values[index];
        }
        
        static quat<T> to_quat(const mat4<T>& mat) {
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
    bool operator==(const mat4<T>& a, const mat4<T>& b) {
        for (auto i = 0u; i < 16u; ++i) {
            if (std::abs(a[i] - b[i]) > std::numeric_limits<T>::epsilon()) {
                return false;
            }
        }
        return true;
    }

    template<typename T>
    bool operator!=(const mat4<T>& a, const mat4<T>& b) {
        return !(a == b);
    }

    template<typename T>
    constexpr T determinant(const mat4<T>& mat) {
        if (mat == mat4<T>::identity()) {
            return 1;
        }

        const T m00 = mat[0], m01 = mat[1], m02 = mat[2], m03 = mat[3];
        const T m10 = mat[4], m11 = mat[5], m12 = mat[6], m13 = mat[7];
        const T m20 = mat[8], m21 = mat[9], m22 = mat[10], m23 = mat[11];
        const T m30 = mat[12], m31 = mat[13], m32 = mat[14], m33 = mat[15];

        const T det_22_33 = m22 * m33 - m32 * m23;
        const T det_21_33 = m21 * m33 - m31 * m23;
        const T det_21_32 = m21 * m32 - m31 * m22;
        const T det_20_33 = m20 * m33 - m30 * m23;
        const T det_20_32 = m20 * m32 - m22 * m30;
        const T det_20_31 = m20 * m31 - m30 * m21;
        const T cofact_00 = +(m11 * det_22_33 - m12 * det_21_33 + m13 * det_21_32);
        const T cofact_01 = -(m10 * det_22_33 - m12 * det_20_33 + m13 * det_20_32);
        const T cofact_02 = +(m10 * det_21_33 - m11 * det_20_33 + m13 * det_20_31);
        const T cofact_03 = -(m10 * det_21_32 - m11 * det_20_32 + m12 * det_20_31);
        return m00 * cofact_00 + m01 * cofact_01 + m02 * cofact_02 + m03 * cofact_03;
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
    constexpr vec3<T> operator*(const mat4<T>& a, const vec3<T>& b) {
        vec4<T> vec;
        vec.x = b.x * a[0] + b.y * a[4] + b.z * a[8] + a[12];
        vec.y = b.x * a[1] + b.y * a[5] + b.z * a[9] + a[13];
        vec.z = b.x * a[2] + b.y * a[6] + b.z * a[10] + a[14];
        vec.w = b.x * a[3] + b.y * a[7] + b.z * a[11] + a[15];
        return { vec.x / vec.w, vec.y / vec.w, vec.z / vec.w };
    }

    template<typename T>
    constexpr vec4<T> operator*(const mat4<T>& a, const vec4<T>& b) {
        return {
            b.x * a[0] + b.y * a[4] + b.z * a[8] + b.w * a[12],
            b.x * a[1] + b.y * a[5] + b.z * a[9] + b.w * a[13],
            b.x * a[2] + b.y * a[6] + b.z * a[10] + b.w * a[14],
            b.x * a[3] + b.y * a[7] + b.z * a[11] + b.w * a[15]
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

    template<typename T>
    vec3<T> unproject(const vec3<T>& vec, const vec4<T>& viewport, const mat4<T>& mat) {
        return mat * vec3<T>{
            (((vec.x - viewport.x) / viewport.z) * 2) - 1,
            -((((vec.y - viewport.y) / viewport.w) * 2) - 1),
            vec.z // (vector.z - minZ) / (maxZ - minZ)
        };
    }

	using mat4f = mat4<float>;
}
