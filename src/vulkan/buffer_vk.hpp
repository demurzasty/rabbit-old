#pragma once 

#include <rabbit/buffer.hpp>

namespace rb {
    class buffer_vk : public buffer {
    public:
        buffer_vk(const buffer_desc& desc);

        ~buffer_vk();

        void* map() override;

        void unmap() override;

    protected:
        void update(const void* data, std::size_t size) override;
    };
}