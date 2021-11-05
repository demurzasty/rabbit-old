#pragma once 

#include "assets.hpp"
#include "vec2.hpp"
#include "vec3.hpp"
#include "vec4.hpp"
#include "color.hpp"
#include "json.hpp"

#include <type_traits>

// TODO: Generic vec2/vec3/vec4 support.

namespace rb {
	class json_read_visitor {
	public:
		void operator()(const char* name, vec2u& data) {
			if (json.contains(name)) {
				const auto& array = json[name];
				data = { array[0], array[1] };
			}
		}

		void operator()(const char* name, vec3u& data) {
			if (json.contains(name)) {
				const auto& array = json[name];
				data = { array[0], array[1], array[2] };
			}
		}

		void operator()(const char* name, vec4u& data) {
			if (json.contains(name)) {
				const auto& array = json[name];
				data = { array[0], array[1], array[2], array[3] };
			}
		}

		void operator()(const char* name, vec2i& data) {
			if (json.contains(name)) {
				const auto& array = json[name];
				data = { array[0], array[1] };
			}
		}

		void operator()(const char* name, vec3i& data) {
			if (json.contains(name)) {
				const auto& array = json[name];
				data = { array[0], array[1], array[2] };
			}
		}

		void operator()(const char* name, vec4i& data) {
			if (json.contains(name)) {
				const auto& array = json[name];
				data = { array[0], array[1], array[2], array[3] };
			}
		}

		void operator()(const char* name, vec2f& data) {
			if (json.contains(name)) {
				const auto& array = json[name];
				data = { array[0], array[1] };
			}
		}

		void operator()(const char* name, vec3f& data) {
			if (json.contains(name)) {
				const auto& array = json[name];
				data = { array[0], array[1], array[2] };
			}
		}

		void operator()(const char* name, vec4f& data) {
			if (json.contains(name)) {
				const auto& array = json[name];
				data = { array[0], array[1], array[2], array[3] };
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