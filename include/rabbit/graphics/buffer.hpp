#pragma once

#include "../core/config.hpp"

#include <cstddef>

namespace rb {
    enum class buffer_type {
        vertex,
        index,
        uniform
    };

    struct buffer_desc {
        buffer_type type{ buffer_type::vertex };
        const void* data{ nullptr };
        std::size_t size{ 0 };
        std::size_t stride{ 0 };
    };

    class buffer {
    public:
        virtual ~buffer() = default;

        virtual void update(const void* data) = 0;

        RB_NODISCARD buffer_type type() const RB_NOEXCEPT;

        RB_NODISCARD std::size_t size() const RB_NOEXCEPT;

        RB_NODISCARD std::size_t stride() const RB_NOEXCEPT;

        RB_NODISCARD std::size_t count() const RB_NOEXCEPT;

    protected:
        buffer(const buffer_desc& desc);

    private:
        const buffer_type _type;
        const std::size_t _size;
        const std::size_t _stride;
    };
}
