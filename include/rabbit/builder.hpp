#pragma once 

#include "application.hpp"
#include "config.hpp"
#include "di.hpp"
#include "system.hpp"
#include "asset_manager.hpp"

#include <list>
#include <memory>
#include <functional>
#include <type_traits>

namespace rb {
    class builder {
        friend class application;

    public:
        template<typename T>
        builder& singleton() {
            _installations.push_back([](injector& injector) {
                injector.install<T>();
            });
            return *this;
        }

        template<typename Interface, typename Implementation>
        builder& singleton() {
            _installations.push_back([](injector& injector) {
                injector.install<Interface, Implementation>();
            });
            return *this;
        }

        template<typename Interface, typename Func>
        builder& singleton(Func func) {
            _installations.push_back([func](injector& injector) {
                injector.install<Interface>(func);
            });
            return *this;
        }


        template<typename Asset, typename Loader>
        builder& loader() {
            _installations.push_back([](injector& injector) {
                injector.invoke([](asset_manager& asset_manager) {
                    asset_manager.add_loader<Asset, Loader>();
                });
            });
            return *this;
        }

        template<typename System>
        builder& system() {
            _system_factories.push_back([](injector& injector) {
                return injector.create<std::shared_ptr<System>>();
            });
            return *this;
        }
        
        RB_NODISCARD application build() const;

    private:
        std::list<std::function<void(injector&)>> _installations;
        std::list<std::function<std::shared_ptr<rb::system>(injector&)>> _system_factories;
    };

    RB_NODISCARD builder make_builder();
}
