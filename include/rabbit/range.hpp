#pragma once 

#include "config.hpp"

namespace rb {
    template<typename T>
    struct range_iterator {
        T value{};

        range_iterator<T>& operator++() RB_NOEXCEPT {
            ++value;
            return *this;
        }

        range_iterator<T> operator++(int) RB_NOEXCEPT  {
            const auto temp = *this;
            ++value;
            return temp;
        }

        RB_NODISCARD T operator*() const RB_NOEXCEPT {
            return value;
        }

        RB_NODISCARD bool operator==(const range_iterator<T>& rhs) const RB_NOEXCEPT {
            return value == rhs.value;
        }

        RB_NODISCARD bool operator!=(const range_iterator<T>& rhs) const RB_NOEXCEPT {
            return value != rhs.value;
        }
    };

    template<typename T>
    struct range {
        const T min;
        const T max;

        RB_NODISCARD range_iterator<T> begin() { return { min }; }
        RB_NODISCARD range_iterator<T> end() { return { max }; };

        RB_NODISCARD range_iterator<T> begin() const { return { min }; }
        RB_NODISCARD range_iterator<T> end() const { return { max }; };
    };

    template<typename T>
    RB_NODISCARD range<T> make_range(const T& min, const T& max) RB_NOEXCEPT {
        return range<T>{ min, max };
    }
}
