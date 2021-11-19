#pragma once 

#include <rabbit/engine/math/vec2.hpp>
#include <rabbit/engine/core/json.hpp>
#include <rabbit/engine/core/bstream.hpp>
#include <rabbit/engine/core/fnv1a.hpp>

#include <memory>
#include <string>

namespace rb {
	struct environment_desc {
		const void* data{ nullptr };
		vec2u size{ 0, 0 };
	};

	class environment {
	public:
		static constexpr auto magic_number{ fnv1a("environment") };

		static std::shared_ptr<environment> load(ibstream& stream);

		static void import(ibstream& input, obstream& output, const json& metadata);

		virtual ~environment() = default;

		const vec2u& size() const;

	protected:
		environment(const environment_desc& desc);

	private:
		const vec2u _size;
	};
}
