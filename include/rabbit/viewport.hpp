#pragma once 

#include "vec2.hpp"

namespace rb {
    struct viewport_desc {
        vec2u size{ 0, 0 };
    };

    class viewport {
    public:
        virtual ~viewport() = default;

        const vec2u& size() const;

    protected:
        viewport(const viewport_desc& desc);

    private:
        const vec2u _size;
    };
}
