#pragma once 

#include <gsl/gsl>

namespace rb {
    template<typename T>
    using span = gsl::span<T>;
}
