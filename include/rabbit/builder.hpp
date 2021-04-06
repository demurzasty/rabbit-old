#pragma once 

#include "container.hpp"
#include "system.hpp"
#include "loader.hpp"

#include <vector>
#include <functional>

namespace rb {
    class builder {
        friend class application;

    public:
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

        template<typename Interface, typename Func>
        builder& service(Func factory) {
            _services.push_back([func](container& container) {
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

    private:
        std::vector<std::function<void(container&)>> _services;
        std::vector<std::function<std::shared_ptr<rb::system>(container&)>> _systems;
        std::vector<std::function<void(container&)>> _initializers;
    };
}
