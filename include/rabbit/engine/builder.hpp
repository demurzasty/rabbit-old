#pragma once 

#include "../core/container.hpp"
#include "../entity/system.hpp"
#include "../asset/loader.hpp"

#include <vector>
#include <functional>

namespace rb {
    class builder {
        friend class game;

    public:
        builder() = default;

        builder(const builder&) = delete;
        builder(builder&&) = default;

        builder& operator=(const builder&) = delete;
        builder& operator=(builder&&) = default;

        template<typename T>
        builder& service() {
            _services.push_back([](container& container) {
                container.install<T>();
            });
            return *this;
        }

        template<typename T>
        builder& service(std::shared_ptr<T> instance) {
            _services.push_back([instance](container& container) {
                container.install<T>(instance);
            });
            return *this;
        }

        template<typename Interface, typename Implementation>
        builder& service() {
            _services.push_back([](container& container) {
                container.install<Interface, Implementation>();
            });
            return *this;
        }

        template<typename Interface, typename Factory>
        builder& service(Factory factory) {
            _services.push_back([factory](container& container) {
                container.install<Interface>(factory);
            });
            return *this;
        }

        template<typename System>
        builder& system() {
            _systems.push_back([](container& container) {
                return container.resolve<System>();
            });
            return *this;
        }

        template<typename Func>
        builder& initialize(Func func) {
            _initializers.push_back([func](container& container) {
                container.invoke(func);
            });
            return *this;
        }

        game build();

    private:
        std::vector<std::function<void(container&)>> _services;
        std::vector<std::function<std::shared_ptr<rb::system>(container&)>> _systems;
        std::vector<std::function<void(container&)>> _initializers;
    };

    [[nodiscard]] builder make_default_builder();
}
