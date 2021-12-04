#pragma once 

#include "../core/entity.hpp"
#include "../math/vec3.hpp"
#include "../math/mat4.hpp"

namespace rb {
    struct local_transform {
        entity parent{ null };
        vec3f position{ 0.0f, 0.0f, 0.0f };
        vec3f rotation{ 0.0f, 0.0f, 0.0f };
        vec3f scale{ 1.0f, 1.0f, 1.0f };
    };

    struct global_transform {
        vec3f position{ 0.0f, 0.0f, 0.0f };
        vec3f rotation{ 0.0f, 0.0f, 0.0f };
        vec3f scale{ 1.0f, 1.0f, 1.0f };

        void mark_as_dirty() {
            _dirty = true;
        }

        const mat4f& world_matrix() const noexcept {
            if (_dirty) {
                _world_matrix = mat4f::translation(position) *
                    mat4f::rotation(rotation) *
                    mat4f::scaling(scale);
                _dirty = false;
            }
            return _world_matrix;
        }

    private:
        mutable bool _dirty{ true };
        mutable mat4f _world_matrix{ mat4f::identity() };
    };
}
