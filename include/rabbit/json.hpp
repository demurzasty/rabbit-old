#pragma once 

#include "assets.hpp"
#include "vec3.hpp"
#include "color.hpp"

#include <nlohmann/json.hpp>

#include <type_traits>

namespace rb {
	using nlohmann::json;

	class json_read_visitor {
	public:
		void operator()(const char* name, vec3f& data) {
			if (json.contains(name)) {
				const auto& array = json[name];
				data = { array[0], array[1], array[2] };
			}
		}

		void operator()(const char* name, color& data) {
			if (json.contains(name)) {
				const auto& array = json[name];
				data = { array[0], array[1], array[2], array[3] };
			}
		}

		template<typename T, std::enable_if_t<std::is_same_v<std::decay_t<T>, std::string>, int> = 0>
		void operator()(const char* name, T& data) {
			if (json.contains(name)) {
				data = json[name];
			}
		}

		template<typename T, std::enable_if_t<std::is_fundamental_v<T>, int> = 0>
		void operator()(const char* name, T& data) {
			if (json.contains(name)) {
				data = json[name];
			}
		}

		template<typename T, std::enable_if_t<std::is_class_v<T>, int> = 0>
		void operator()(const char* name, std::shared_ptr<T>& data) {
			if (json.contains(name)) {
				if (const auto uuid = uuid::from_string(json[name]); uuid) {
					data = assets::load<T>(*uuid);
				}
			}
		}

		template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0, typename... Args>
		void operator()(const char* name, T& data, Args&&... args) {
			if (json.contains(name)) {
				std::array<const char*, sizeof...(Args)> names{ args... };
				std::string value_name = json[name];

				for (std::size_t i{ 0 }; i < names.size(); ++i) {
					if (value_name == names[i]) {
						data = static_cast<T>(i);
						break;
					}
				}
			}
		}

	public:
		json& json;
	};
}
