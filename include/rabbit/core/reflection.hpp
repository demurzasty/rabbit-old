#pragma once 

#include "json.hpp"
#include "variant.hpp"

#include <string>
#include <vector>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <optional>
#include <string_view>

namespace rb {
    struct field {
        template<typename U, typename T, std::enable_if_t<variant::helpers::is_acceptable_v<U>, int> = 0>
        void set(T& obj, const U& value) const {
            setter(&obj, value);
        }

        template<typename U, typename T, std::enable_if_t<variant::helpers::is_acceptable_v<U>, int> = 0>
        U get(const T& obj) const {
            return getter(&obj);
        }

        void (*setter)(void*, const variant&);
        variant(*getter)(const void*);
    };

    struct type {
        std::string name;
        std::unordered_map<std::string, field> fields;
        std::unordered_map<std::string, variant> values;
    };

    template<typename T>
    class meta {
    public:
        meta(type& type) : _type(type) {}

        meta<T>& name(const std::string_view& name) {
            _type.name = name;
            return *this;
        }

        template<auto F>
        meta<T>& field(const std::string_view& name) {
            rb::field field;
            field.setter = [](void* data, const variant& variant) {
                auto ptr = static_cast<T*>(data);
                ptr->*F = variant;
            };
            field.getter = [](const void* data) -> variant {
                const auto ptr = static_cast<const T*>(data);
                return ptr->*F;
            };
            _type.fields.emplace(name, field);
            return *this;
        }

        template<auto Value>
        meta<T>& value(const std::string_view& name) {
            _type.values.emplace(name, Value);
            return *this;
        }

    private:
        rb::type& _type;
    };

    class reflection {
    public:
        template<typename T>
        static meta<T> reflect() {
            return { _types[typeid(T)] };
        }

        template<typename T>
        static const rb::type& type() {
            return _types.at(typeid(T));
        }


        static void deserialize(std::type_index type_index, void* data, const json& json);

    private:
        static std::unordered_map<std::type_index, rb::type> _types;
    };
}
