#pragma once 

#include <rabbit/resource_heap.hpp>

#include <volk.h>

#include <vector>

namespace rb {
    class resource_heap_vulkan : public resource_heap {
    public:
        resource_heap_vulkan(VkDevice device, const resource_heap_desc& desc);

        ~resource_heap_vulkan();
    
        RB_NODISCARD const std::vector<VkDescriptorSet>& descriptor_sets() const RB_NOEXCEPT;

    private:
        void _create_descriptor_pool(const resource_heap_desc& desc, const std::vector<material_binding_desc>& bindings);
    
        void _create_descriptor_sets(const resource_heap_desc& desc);
        
        void _update_descriptor_sets(const resource_heap_desc& desc, const std::vector<material_binding_desc>& bindings);

    private:
        VkDevice _device;
        VkDescriptorPool _descriptor_pool;
        std::vector<VkDescriptorSet> _descriptor_sets;
    };
}
