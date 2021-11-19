#pragma once 

#include <rabbit/engine/core/variant.hpp>

#include <string>
#include <vector>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <optional>

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
        std::unordered_map<std::string, field> fields;
    };

    template<typename T>
    class meta {
    public:
        meta(type& type) : _type(type) {}

        template<auto F>
        meta<T>& field(const std::string& name) {
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

    private:
        static std::unordered_map<std::type_index, rb::type> _types;
    };
}
