#pragma once 

#include <nlohmann/json.hpp>

namespace rb {
	using nlohmann::json;

	class json_write_visitor {
	public:
		template<typename T>
		void operator()(const char* name, const T& data) {
			json[name] = data;
		}

	public:
		json& json;
	};
}
