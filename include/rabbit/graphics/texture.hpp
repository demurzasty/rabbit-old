#pragma once 

#include "../core/span.hpp"
#include "../math/vec2.hpp"
#include "color.hpp"

#include <vector>
#include <string>
#include <memory>
#include <cstdint>

namespace rb {
	enum class texture_format {
		r8,
		rg8,
		rgba8,
		bc1,
		bc3
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
		std::vector<std::uint8_t> pixels;
		vec2u size{ 0, 0 };
		texture_format format{ texture_format::rgba8 };
		texture_filter filter{ texture_filter::linear };
		texture_wrap wrap{ texture_wrap::repeat };
		std::uint32_t mipmaps{ 0 };
	};

	class texture_utils {
	public:
		static std::uint32_t mipmap_levels(const vec2u& texture_size);

		static std::uint32_t bits_per_pixel(texture_format format);
	};

	class texture {
	public:
		virtual ~texture() = default;

		const vec2u& size() const noexcept;

		texture_format format() const noexcept;

		texture_filter filter() const noexcept;

		texture_wrap wrap() const noexcept;

		std::uint32_t mipmaps() const noexcept;

		std::uint32_t bits_per_pixel() const noexcept;

	protected:
		explicit texture(const texture_desc& desc);

	private:
		const vec2u _size;
		const texture_format _format;
		const texture_filter _filter;
		const texture_wrap _wrap;
		const std::uint32_t _mipmaps;
		const std::uint32_t _bits_per_pixel;
	};
}