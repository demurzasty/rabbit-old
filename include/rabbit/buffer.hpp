#pragma once 

#include "span.hpp"
#include "buffer_type.hpp"

#include <cstdint>
#include <cstddef>

namespace rb {
    struct buffer_desc {
        buffer_type type = buffer_type::unknown;
        std::size_t size = 0;
        const void* data = nullptr;
        bool is_mutable = false;
    };

    class buffer {
    public:
        virtual ~buffer() = default;

        buffer_type type() const;

        std::size_t size() const;

        std::size_t stride() const;

        std::size_t count() const;

        bool is_mutable() const;

    protected:
        buffer(const buffer_desc& desc);

    private:
        buffer_type _type;
        std::size_t _size;
        bool _is_mutable;
    };
}
