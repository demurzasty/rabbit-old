#pragma once 

#include "vec2.hpp"
#include "json.hpp"

#include <memory>
#include <string>
namespace rb {
	struct environment_desc {
		const void* data{ nullptr };
		vec2u size{ 0, 0 };
	};

	class environment {
	public:
		static std::shared_ptr<environment> load(const std::string& filename, json& metadata);

		static void import(const std::string& input, const std::string& output, const json& metadata);

		virtual ~environment() = default;

		const vec2u& size() const;

	protected:
		environment(const environment_desc& desc);

	private:
		const vec2u _size;
	};
}
