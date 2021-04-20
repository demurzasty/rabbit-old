#pragma once

#include "config.hpp"
#include "type_info.hpp"

#include <map>
#include <memory>
#include <functional>
#include <type_traits>

namespace rb {
    class injector;

    /**
     * @brief Dependency installer used by dependency injector.
     */
    template<typename Interface>
    struct installer {
        template<typename Implementation>
        void install();

        injector& injector;
    };

    /**
     * @brief Dependency injector. 
     */
    class injector {
        struct bean {
            std::shared_ptr<void> instance;
            std::function<std::shared_ptr<void>(injector&)> factory;
        };

        /**
         * @brief Automatic resolver helper structure.
         */
        template<typename T>
        struct resolver {
            template<typename U, std::enable_if_t<!std::is_same_v<std::decay_t<T>, std::decay_t<U>>, int> = 0>
            operator U&() {
                return injector.get<U>();
            }

            injector& injector;
        };

    public:
        /**
         * @brief Install type.
         */
        template<typename T>
        injector& install() {
            _beans.emplace(type_id<T>().hash(), bean{
                nullptr,
                [](injector& injector) -> std::shared_ptr<void> {
                    return injector.resolve<T>();
                }
            });
            return *this;
        }

        /**
         * @brief Install type.
         */
        template<typename T>
        injector& install(std::shared_ptr<T> instance) {
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
        injector& install() {
            _beans.emplace(type_id<Interface>().hash(), bean{
                nullptr,
                [](injector& injector) -> std::shared_ptr<void> {
                    return injector.resolve<Implementation>();
                }
            });
            return *this;
        }

        template<typename Interface, typename Func>
        injector& install(Func func) {
            func(installer<Interface>{ *this });
            return *this;
        }

        template<typename T>
        RB_NODISCARD std::shared_ptr<T> resolve() {
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
            } else {
                static_assert(false, "No suitable constructor found");
            }
        }

        template<typename Func>
        RB_NODISCARD void invoke(Func func) {
            using resolver = resolver<void>;

            resolver r{ *this };
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
            } else {
                static_assert(false, "No suitable invocation found");
            }
        }

        template<typename T, std::enable_if_t<!std::is_same_v<std::decay_t<T>, injector>, int> = 0>
        RB_NODISCARD T& get() {
            RB_ASSERT(installed<T>(), "Type is not installed");

            auto& bean = _beans.at(type_id<T>().hash());
            if (!bean.instance) {
                bean.instance = bean.factory(*this);
            }
            return *std::static_pointer_cast<T>(bean.instance);
        }

        template<typename T, std::enable_if_t<std::is_same_v<std::decay_t<T>, injector>, int> = 0>
        RB_NODISCARD T& get() {
            return *this;
        }

        template<typename T>
        RB_NODISCARD bool installed() const {
            return _beans.find(type_id<T>().hash()) != _beans.end();
        }

    private:
        std::map<id_type, bean> _beans;
    };

    template<typename Interface>
    template<typename Implementation>
    void installer<Interface>::install() {
        injector.install<Interface, Implementation>();
    }
}