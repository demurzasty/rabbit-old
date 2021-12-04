#pragma once 

#include "entity.hpp"

#include <cstdint>

namespace rb {
    struct system_execution_flags {
        static constexpr std::uint32_t runtime_bit{ 1 << 0 };
        static constexpr std::uint32_t editor_bit{ 1 << 1 };
    };

    enum class system_stage {
        init,
        update,
        fixed_update
    };

    class system {
    public:
        virtual ~system() = default;

        virtual void process() = 0;

        virtual system_stage stage() const noexcept;

        virtual std::uint32_t execution_flags() const noexcept;

        virtual int priority() const noexcept;
    };
}
