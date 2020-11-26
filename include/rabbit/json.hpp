#pragma once 

#include "json_fwd.hpp"
#include "enum.hpp"

#include <nlohmann/json.hpp>

#include <optional>
#include <type_traits>

namespace rb {
    namespace detail {
        template<typename T>
        bool json_valid(const json& json) {
            return false;
        }

        template<> inline bool json_valid<bool>(const json& json) { return json.is_boolean(); }
        template<> inline bool json_valid<std::uint8_t>(const json& json) { return json.is_number_unsigned(); }
        template<> inline bool json_valid<std::uint16_t>(const json& json) { return json.is_number_unsigned(); }
        template<> inline bool json_valid<std::uint32_t>(const json& json) { return json.is_number_unsigned(); }
        template<> inline bool json_valid<std::uint64_t>(const json& json) { return json.is_number_unsigned(); }
        template<> inline bool json_valid<std::int8_t>(const json& json) { return json.is_number_integer(); }
        template<> inline bool json_valid<std::int16_t>(const json& json) { return json.is_number_integer(); }
        template<> inline bool json_valid<std::int32_t>(const json& json) { return json.is_number_integer(); }
        template<> inline bool json_valid<std::int64_t>(const json& json) { return json.is_number_integer(); }
        template<> inline bool json_valid<float>(const json& json) { return json.is_number(); } // allow reading integer from json into floating points
        template<> inline bool json_valid<double>(const json& json) { return json.is_number(); } // allow reading integer from json into floating points
        template<> inline bool json_valid<long double>(const json& json) { return json.is_number(); } // allow reading integer from json into floating points
        template<> inline bool json_valid<std::string>(const json& json) { return json.is_string(); }

        // for non-enum types
        template<typename T, std::enable_if_t<!std::is_enum_v<T>, int> = 0>
        inline std::optional<T> json_value(const json& member) {
            // test if json member is convertible to T
            if (detail::json_valid<T>(member)) {
                return{ member };
            }
            return {};
        }

        // for enum types
        template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
        inline std::optional<T> json_value(const json& member) {
            return enum_value<T>(member);
        }
    }

    template<typename T>
    std::optional<T> json_utils::member(const json& json, const std::string& member_name) {
        if (!json.contains(member_name)) {
            return {};
        }

        return detail::json_value<T>(json[member_name]);
    }

    template<typename T>
    T json_utils::member_or(const json& json, const std::string& member_name, const T& value) {
        return member<T>(json, member_name).value_or(value);
    }
}
