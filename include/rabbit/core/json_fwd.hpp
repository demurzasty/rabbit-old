#pragma once 

#include <nlohmann/json_fwd.hpp>

#include <string>
#include <optional>

namespace rb {
    using json = nlohmann::json;

    struct json_utils {
        template<typename T>
        static std::optional<T> member(const json& json, const std::string& member_name);

        template<typename T>
        static T member_or(const json& json, const std::string& member_name, const T& value);
    };
}
