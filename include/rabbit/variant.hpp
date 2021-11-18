#pragma once 

#include "vec2.hpp"
#include "vec3.hpp"
#include "vec4.hpp"
#include "mat4.hpp"
#include "quat.hpp"

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <variant>

// TODO: Auto conversion? Convertible?

namespace rb {
    /**
     * @brief Holds any value that is known by engine.
     *        Basically it will be used for serialization and reflection system used by engine.
     */
    class variant {
        using internal_variant_type = std::variant<
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

    public:
        variant() = default;

        variant(const variant&) = default;
        variant(variant&&) = default;

        template<typename T>
        variant(const T& data)
            : _data(data) {
        }

        variant& operator=(const variant&) = default;
        variant& operator=(variant&&) = default;

        template<typename T>
        variant& operator=(const T& data) {
            _data = data;
            return *this;
        }

        template<typename T>
        operator const T& () const {
            return std::get<T>(_data);
        }

        template<typename T>
        bool holds() const {
            return std::holds_alternative<T>(_data);
        }

    private:
        internal_variant_type _data;
    };
}
