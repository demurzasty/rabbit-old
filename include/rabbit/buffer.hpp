#pragma once 

#include "span.hpp"
#include "buffer_type.hpp"

#include <cstdint>
#include <cstddef>

namespace rb {
    struct buffer_desc {
        buffer_type type{ buffer_type::vertex };
        std::size_t size{ 0 };
        std::size_t stride{ 0 };
        const void* data{ nullptr };
        bool is_mutable{ false };
    };

    class buffer {
    public:
        virtual ~buffer() = default;

        virtual void* map() = 0;

        virtual void unmap() = 0;

        template<typename T>
        void update(const span<T>& data) {
            update(data.data(), data.size_bytes());
        }

        buffer_type type() const;

        std::size_t size() const;

        std::size_t stride() const;

        std::size_t count() const;

        bool is_mutable() const;

    protected:
        buffer(const buffer_desc& desc);

        virtual void update(const void* data, std::size_t size) = 0;

    private:
        buffer_type _type;
        std::size_t _size;
        std::size_t _stride;
        bool _is_mutable;
    };
}
