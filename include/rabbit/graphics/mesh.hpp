#pragma once 

#include "vertex.hpp"
#include "buffer.hpp"

#include <cstdint>
#include <memory>

namespace rb {
    enum class index_type {
        uint8,
        uint16,
        uint32
    };

    struct mesh_desc {
        vertex_desc vertex_desc;
        std::shared_ptr<buffer> vertex_buffer;
        index_type index_type{ index_type::uint32 };
        std::shared_ptr<buffer> index_buffer;
    };

    class mesh {
    public:
        mesh(const mesh_desc& desc);

        virtual ~mesh() = default;
    
        RB_NODISCARD const vertex_desc& vertex_desc() const RB_NOEXCEPT;

        RB_NODISCARD const std::shared_ptr<buffer>& vertex_buffer() const RB_NOEXCEPT;

        RB_NODISCARD index_type index_type() const RB_NOEXCEPT;

        RB_NODISCARD const std::shared_ptr<buffer>& index_buffer() const RB_NOEXCEPT;

    private:
        rb::vertex_desc _vertex_desc;
        std::shared_ptr<buffer> _vertex_buffer;
        rb::index_type _index_type;
        std::shared_ptr<buffer> _index_buffer;
    };
}
