#pragma once 

#include "span.hpp"
#include "vec2.hpp"
#include "json.hpp"
#include "color.hpp"
#include "bstream.hpp"

#include <string>
#include <memory>

namespace rb {
	enum class texture_format {
		r8,
		rg8,
		rgba8
	};

	enum class texture_filter {
		nearest,
		linear,
	};

	enum class texture_wrap {
		clamp,
		repeat,
	};

	struct texture_desc {
		const void* data{ nullptr };
		vec2u size{ 0, 0 };
		texture_format format{ texture_format::rgba8 };
		texture_filter filter{ texture_filter::linear };
		texture_wrap wrap{ texture_wrap::repeat };
		std::uint32_t mipmaps{ 0 };
	};

	class texture {
	public:
		static std::shared_ptr<texture> load(bstream& stream);

		static void import(const std::string& input, const std::string& output, const json& metadata);

		static std::shared_ptr<texture> make_one_color(const color& color, const vec2u& size);

		virtual ~texture() = default;

		const vec2u& size() const;

		texture_format format() const;

		texture_filter filter() const;

		texture_wrap wrap() const;

		std::uint32_t mipmaps() const;

		std::size_t bytes_per_pixel() const;

	protected:
		texture(const texture_desc& desc);

	private:
		const vec2u _size;
		const texture_format _format;
		const texture_filter _filter;
		const texture_wrap _wrap;
		const std::uint32_t _mipmaps;
		const std::size_t _bytes_per_pixel;
	};
}
