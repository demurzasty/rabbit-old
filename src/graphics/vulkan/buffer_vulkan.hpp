#pragma once 

#include <rabbit/graphics/buffer.hpp>

#include <volk.h>

namespace rb {
    class buffer_vulkan : public buffer {
    public:
        buffer_vulkan(const buffer_desc& desc);

        ~buffer_vulkan();

        void* map() override;

        void unmap() override;

    protected:
        void update(const void* data, std::size_t size) override;
    };
}