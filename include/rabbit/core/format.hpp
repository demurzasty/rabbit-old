#pragma once 

#include <fmt/format.h>

namespace rb {
    using fmt::format;
    using fmt::print;

    template <typename S, typename... Args>
    inline void println(const S& format_str, Args&&... args) {
        print("{}\n", format(format_str, std::forward<Args>(args)...));
    }
}
