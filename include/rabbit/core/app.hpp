#pragma once 

#include "system.hpp"

#include <list>
#include <memory>
#include <functional>
#include <unordered_map>

namespace rb {
    class app {
    public:
		template<typename Submodule>
		static void add_submodule() {
			_setups.push_back(&Submodule::setup);
			_releases.push_front(&Submodule::release);
		}

		template<typename Func>
		static void add_initialization(Func func) {
			_inits.push_back(func);
		}

		template<typename System>
		static void add_system() {
			_systems.push_back([] {
				return std::make_shared<System>();
			});
		}

		static void setup();

		static void run();

	private:
		static void _reflect();

    private:
        static std::list<std::function<void()>> _setups;
        static std::list<std::function<void()>> _inits;
        static std::list<std::function<void()>> _releases;
		static std::list<std::function<std::shared_ptr<rb::system>()>> _systems;
    };
}
