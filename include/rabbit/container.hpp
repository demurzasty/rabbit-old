#pragma once

#include "type_info.hpp"
#include "exception.hpp"

#include <map>
#include <memory>
#include <functional>
#include <type_traits>

namespace rb {
    /**
     * @brief Dependency container. 
     */
    class container {
        struct bean {
            std::shared_ptr<void> instance;
            std::function<std::shared_ptr<void>(container&)> factory;
        };

        /**
         * @brief Automatic resolver helper structure.
         */
        template<typename T>
        struct resolver {
            template<typename U, std::enable_if_t<!std::is_same_v<std::decay_t<T>, std::decay_t<U>>, int> = 0>
            operator U&() {
                return container.get<U>();
            }

            container& container;
        };

    public:
        /**
         * @brief Install type.
         */
        template<typename T>
        container& install() {
            _beans.emplace(type_id<T>().hash(), bean{
                nullptr,
                [](container& container) {
                    return container.resolve<T>();
                }
            });
            return *this;
        }

        /**
         * @brief Install type.
         */
        template<typename T>
        container& install(std::shared_ptr<T> instance) {
            _beans.emplace(type_id<T>().hash(), bean{
                instance,
                nullptr
            });
            return *this;
        }

        /**
         * @brief Install type.
         */
        template<typename Interface, typename Implementation>
        container& install() {
            _beans.emplace(type_id<Interface>().hash(), bean{
                nullptr,
                [](container& container) {
                    return container.resolve<Implementation>();
                }
            });
            return *this;
        }

        template<typename Interface, typename Func>
        container& install(Func func) {
            _beans.emplace(type_id<Interface>().hash(), bean{
                nullptr,
                [func](container& container) {
                    return func(container);
                }
            });
            return *this;
        }

        template<typename T>
        [[nodiscard]] std::shared_ptr<T> resolve() {
            using resolver = resolver<T>;

            resolver r{ *this };
            if constexpr (std::is_constructible_v<T>) {
                return std::make_shared<T>();
            } else if constexpr (std::is_constructible_v<T, resolver>) {
                return std::make_shared<T>(r);
            } else if constexpr (std::is_constructible_v<T, resolver, resolver>) {
                return std::make_shared<T>(r, r);
            } else if constexpr (std::is_constructible_v<T, resolver, resolver, resolver>) {
                return std::make_shared<T>(r, r, r);
            } else if constexpr (std::is_constructible_v<T, resolver, resolver, resolver, resolver>) {
                return std::make_shared<T>(r, r, r, r);
            }

            throw make_exception("No suitable constructor found");
        }

        template<typename Func>
        [[nodiscard]] void invoke(Func func) {
            using resolver = resolver<void>;

            resolver r;
            if constexpr (std::is_invocable_v<Func>) {
                func();
            } else if constexpr (std::is_invocable_v<Func, resolver>) {
                func(r);
            } else if constexpr (std::is_invocable_v<Func, resolver, resolver>) {
                func(r, r);
            } else if constexpr (std::is_invocable_v<Func, resolver, resolver, resolver>) {
                func(r, r, r);
            } else if constexpr (std::is_invocable_v<Func, resolver, resolver, resolver, resolver>) {
                func(r, r, r, r);
            }

            throw make_exception("No suitable invocation found");
        }

        template<typename T>
        [[nodiscard]] T& get() {
            auto& bean = _beans.at(type_id<T>().hash());
            if (!bean.instance) {
                bean.instance = bean.factory(*this);
            }
            return *std::static_pointer_cast<T>(bean.instance);
        }

    private:
        std::map<id_type, bean> _beans;
    };
}
