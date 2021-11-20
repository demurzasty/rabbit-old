#pragma once 

#include <rabbit/engine/core/system.hpp>
#include <rabbit/engine/core/json.hpp>
#include <rabbit/engine/core/uuid.hpp>
#include <rabbit/engine/core/visitor.hpp>

#include <list>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>

namespace rb {
	class app {
		using deserializer = void(*)(registry&, entity, json_read_visitor&);

	public:
		static void setup();

		template<typename Submodule>
		static void add_submodule() {
			_preinits.push_back(&Submodule::init);
			_releases.push_front(&Submodule::release);
		}

		template<typename Func>
		static void add_init(Func func) {
			_inits.push_back(func);
		}
		
		template<typename Component>
		static void add_component(const std::string& name) {
			_deserializers.emplace(name, [](registry& registry, entity entity, json_read_visitor& visitor) {
				auto& comp = registry.get_or_emplace<Component>(entity);
				Component::visit(visitor, comp);
			});
		}

		template<typename System>
		static void add_system() {
			_systems.push_back([]() -> std::shared_ptr<rb::system> {
				return std::make_shared<System>();
			});
		}

		static void run(std::string initial_scene = "");

		static deserializer get_deserializer(const std::string& name);

	private:
		static void _main_loop(const std::string& initial_scene);

	private:
		static std::list<void(*)()> _preinits;
		static std::list<void(*)()> _inits;
		static std::list<void(*)()> _releases;
		static std::list<std::shared_ptr<rb::system>(*)()> _systems;
		static std::unordered_map<std::string, deserializer> _deserializers;
	};
}
