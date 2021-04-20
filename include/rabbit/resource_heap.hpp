#pragma once 

#include "material.hpp"
#include "buffer.hpp"

#include <vector>
#include <memory>

namespace rb {
    struct resource_view_desc {
        material_binding_type type{ material_binding_type::uniform_buffer };
        std::shared_ptr<buffer> buffer;
    };

    struct resource_heap_desc {
        std::shared_ptr<material> material;
        std::vector<resource_view_desc> resource_views;
    };

    class resource_heap {
    public:
        resource_heap(const resource_heap_desc& desc);

        virtual ~resource_heap() = default;
    
        RB_NODISCARD const std::shared_ptr<material>& material() const RB_NOEXCEPT;

    private:
        std::shared_ptr<rb::material> _material;
    };
}
