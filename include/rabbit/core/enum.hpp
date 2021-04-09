#pragma once 

#include <string>
#include <cstdint>
#include <optional>
#include <type_traits>

/**
 * @brief Useful macro for enum definition used to convert between string representation and raw integer value.
 * 
 * @code{.cpp}
 * RB_ENUM(my_enum, int, "a", "b", "c")
 * enum class my_enum : int {
 *     a,
 *     b,
 *     c
 * };
 * @endcode
 */
#define RB_ENUM(enum_name, type, ...) enum class enum_name : type; template<> struct rb::enum_def<enum_name> { static inline std::initializer_list<const char*> names() { return { __VA_ARGS__ }; }; };

namespace rb {
    /**
     * @brief Converts enum value to std::size_t.
     */
    template<typename T>
    constexpr std::size_t enum_size(T value) {
        return static_cast<std::size_t>(value);
    }

    /**
     * @brief Structure to generalize using enum types. See RB_ENUM macro.
     */
    template<typename T>
    struct enum_def;

    /**
     * @brief Gets string representation of enum value. Before used, register names using RB_ENUM.
     */
    template<typename T>
    inline const char* enum_name(const T value) {
        return enum_def<T>::names[enum_size(value)];
    }

    /**
     * @brief Gets value from enum string representation. Before used, register names using RB_ENUM.
     */
    template<typename T>
    inline std::optional<T> enum_value(const char* name) {
        using char_traits = std::char_traits<char>;

        const auto names = enum_def<T>::names();

        for (std::size_t i = 0; i < names.size(); ++i) {
            if (char_traits::length(name) == char_traits::length(*(names.begin() + i)) &&
                char_traits::compare(name, *(names.begin() + i), char_traits::length(name)) == 0) {
                return { static_cast<T>(i) };
            }
        }

        return {};
    }

    /**
     * @brief Gets value from enum string representation. Before used, register names using RB_ENUM.
     */
    template<typename T>
    inline std::optional<T> enum_value(const std::string& name) {
        return enum_value<T>(name.c_str());
    }
}
