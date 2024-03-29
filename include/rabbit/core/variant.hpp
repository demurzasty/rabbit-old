#pragma once 

#include "../math/vec2.hpp"
#include "../math/vec3.hpp"
#include "../math/vec4.hpp"
#include "../math/mat4.hpp"
#include "../math/quat.hpp"

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <type_traits>

// TODO: Auto conversion? Convertible?

namespace rb {
    template<typename... Ts>
    struct variant_helpers {
        using variant_type = std::variant<Ts...>;

        template<typename T>
        static constexpr auto is_acceptable_v = std::disjunction_v<std::is_enum<T>, std::is_same<std::decay_t<T>, Ts>...>;
    };

    /**
     * @brief Holds any value that is known by engine.
     *        Basically it will be used for serialization and reflection system used by engine.
     */
    class variant {
    public:
        using helpers = variant_helpers<
            bool,
            int, unsigned int, float,
            vec2i, vec3i, vec4i,
            vec2u, vec3u, vec4u,
            vec2f, vec3f, vec3f,
            mat4f, quatf,
            std::string,
            std::vector<variant>,
            std::map<variant, variant>,
            std::shared_ptr<void>
        >;

        using internal_variant_type = helpers::variant_type;

    public:
        variant() = default;

        variant(const variant&) = default;
        variant(variant&&) = default;

        template<typename T, std::enable_if_t<helpers::is_acceptable_v<T>, int> = 0>
        variant(const T& data) {
            if constexpr (std::is_enum_v<T>) {
                _data = static_cast<int>(data);
            } else {
                _data = data;
            }
        }

        variant& operator=(const variant&) = default;
        variant& operator=(variant&&) = default;

        template<typename T, std::enable_if_t<helpers::is_acceptable_v<T>, int> = 0>
        variant& operator=(const T& data) {
            if constexpr (std::is_enum_v<T>) {
                _data = static_cast<int>(data);
            } else {
                _data = data;
            }
            return *this;
        }

        template<typename T>
        variant& operator=(const std::shared_ptr<T>& data) {
            _data = data;
            return *this;
        }

        template<typename T, std::enable_if_t<helpers::is_acceptable_v<T>, int> = 0>
        operator const T& () const {
            if constexpr (std::is_enum_v<T>) {
                return static_cast<T>(std::get<int>(_data));
            } else {
                return std::get<T>(_data);
            }
        }

        template<typename T>
        operator std::shared_ptr<T> () const {
            return std::static_pointer_cast<T>(std::get<std::shared_ptr<void>>(_data));
        }

        template<typename T, std::enable_if_t<helpers::is_acceptable_v<T>, int> = 0>
        bool holds() const {
            if constexpr (std::is_enum_v<T>) {
                return std::holds_alternative<int>(_data);
            } else {
                return std::holds_alternative<T>(_data);
            }
        }

    private:
        internal_variant_type _data;
    };
}
