#pragma once 

#include <rabbit/engine/math/vec2.hpp>
#include <rabbit/engine/core/entity.hpp>

namespace rb {
    struct viewport_desc {
        vec2u size{ 0, 0 };
    };

    class viewport {
    public:
        virtual ~viewport() = default;

        const vec2u& size() const;

        float aspect() const;

    protected:
        viewport(const viewport_desc& desc);

    public:
        entity camera{ null };
        bool motion_blur_enabled{ true };
        bool fxaa_enabled{ true };
        bool sharpen_enabled{ true };
        float sharpen_factor{ 0.25f };

    private:
        const vec2u _size;
    };
}
