#pragma once 

#include <rabbit/engine/core/span.hpp>
#include <rabbit/engine/math/vec2.hpp>
#include <rabbit/engine/graphics/color.hpp>
#include <rabbit/engine/core/bstream.hpp>
#include <rabbit/engine/core/fnv1a.hpp>

#include <string>
#include <memory>

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
		const void* data{ nullptr };
		vec2u size{ 0, 0 };
		texture_format format{ texture_format::rgba8 };
		texture_filter filter{ texture_filter::linear };
		texture_wrap wrap{ texture_wrap::repeat };
		std::uint32_t mipmaps{ 0 };
	};

	class texture_utils {
	public:
		static std::size_t calculate_mipmap_levels(const vec2u& texture_size);

		static std::size_t calculate_bits_per_pixel(texture_format format);
	};

	class texture {
	public:
		static constexpr auto magic_number{ fnv1a("texture") };

		static std::shared_ptr<texture> load(ibstream& stream);

		static std::shared_ptr<texture> make_one_color(const color& color, const vec2u& size);

		virtual ~texture() = default;

		const vec2u& size() const;

		texture_format format() const;

		texture_filter filter() const;

		texture_wrap wrap() const;

		std::uint32_t mipmaps() const;

		std::size_t bits_per_pixel() const;

	protected:
		texture(const texture_desc& desc);

	private:
		const vec2u _size;
		const texture_format _format;
		const texture_filter _filter;
		const texture_wrap _wrap;
		const std::uint32_t _mipmaps;
		const std::size_t _bits_per_pixel;
	};
}
