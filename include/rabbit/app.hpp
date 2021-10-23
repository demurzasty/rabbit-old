#pragma once 

#include "system.hpp"

#include <list>

namespace rb {
	class app {
	public:
		template<typename Submodule>
		static void submodule() {
			_preinits.push_back(&Submodule::init);
			_releases.push_front(&Submodule::release);
		}

		template<typename Func>
		static void init(Func func) {
			_inits.push_back(func);
		}

		template<typename System>
		static void system() {
			_systems.push_back([]() -> std::shared_ptr<rb::system> {
				return std::make_shared<System>();
			});
		}

		static void setup();

		static void run();

	private:
		static void _main_loop();

	private:
		static std::list<void(*)()> _preinits;
		static std::list<void(*)()> _inits;
		static std::list<void(*)()> _releases;
		static std::list<std::shared_ptr<rb::system>(*)()> _systems;
	};
}
