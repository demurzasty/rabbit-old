#include <rabbit/resource_heap.hpp>

#include <rabbit/range.hpp>

using namespace rb;

resource_heap::resource_heap(const resource_heap_desc& desc)
    : _material(desc.material) {
    RB_ASSERT(_material, "Material is not provided");
    RB_ASSERT(!_material->bindings().empty(), "Cannot create resource heap without material bindings");
    RB_ASSERT(desc.resource_views.size() % _material->bindings().size() == 0, "Failed to create resource heap because number of resource views is not a multiple of bindings in material layout");

    for (auto index : rb::make_range<std::size_t>(0u, desc.resource_views.size())) {
        const auto resource_type = desc.resource_views[index].type;
        const auto binding_type = _material->bindings()[index % _material->bindings().size()].binding_type;
        RB_ASSERT(resource_type == binding_type, "Resource heap layout doesn't match with material layout");
    }
}
   
const std::shared_ptr<material>& resource_heap::material() const RB_NOEXCEPT {
    return _material;
}