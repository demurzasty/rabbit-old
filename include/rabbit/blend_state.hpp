#pragma once 

#include "enum.hpp"
#include "color.hpp"

#include <cstdint>

namespace rb {
	RB_ENUM(blend, std::uint8_t, "one", "zero", "source_color", "inverse_source_color", "source_alpha", "inverse_source_alpha",
		"destination_color", "inverse_destination_color", "blend_factor", "inverse_blend_factor", "source_alpha_saturation")
	enum class blend : std::uint8_t {
		// Each component of the color is multiplied by (1, 1, 1, 1).
		one,

		// Each component of the color is multiplied by (0, 0, 0, 0).
		zero, 

		// Each component of the color is multiplied by the source color.
		// This can be represented as (Rs, Gs, Bs, As), where R, G, B, and A
		// respectively stand for the red, green, blue, and alpha source values.
		source_color,

		// Each component of the color is multiplied by the inverse of the source color.
		// This can be represented as (1 − Rs, 1 − Gs, 1 − Bs, 1 − As) where R, G, B, and A
		// respectively stand for the red, green, blue, and alpha destination values.
		inverse_source_color,

		// Each component of the color is multiplied by the alpha value of the source.
		// This can be represented as (As, As, As, As), where As is the alpha source value.
		source_alpha,

		// Each component of the color is multiplied by the inverse of the alpha value of the source.
		// This can be represented as (1 − As, 1 − As, 1 − As, 1 − As), where As is the alpha destination value.
		inverse_source_alpha,

		// Each component color is multiplied by the destination color.
		// This can be represented as (Rd, Gd, Bd, Ad), where R, G, B, and A
		// respectively stand for red, green, blue, and alpha destination values.
		destination_color,

		// Each component of the color is multiplied by the inverse of the destination color.
		// This can be represented as (1 − Rd, 1 − Gd, 1 − Bd, 1 − Ad), where Rd, Gd, Bd, and Ad
		// respectively stand for the red, green, blue, and alpha destination values.
		inverse_destination_color,

		// Each component of the color is multiplied by the alpha value of the destination.
		// This can be represented as (Ad, Ad, Ad, Ad), where Ad is the destination alpha value.
		destination_alpha,

		// 	Each component of the color is multiplied by the inverse of the alpha value of the destination. 
		// This can be represented as (1 − Ad, 1 − Ad, 1 − Ad, 1 − Ad), where Ad is the alpha destination value.
		inverse_destination_alpha,

		// Each component of the color is multiplied by a constant set in BlendFactor.
		blend_factor,

		// Each component of the color is multiplied by the inverse of a constant set in BlendFactor.
		inverse_blend_factor,

		// Each component of the color is multiplied by either the alpha of the source color, 
		// or the inverse of the alpha of the source color, whichever is greater.
		// This can be represented as (f, f, f, 1), where f = min(A, 1 − Ad).
		source_alpha_saturation
	};

	RB_ENUM(blend_function, std::uint8_t, "add", "subtract", "reverse_subtract", "min", "max")
	enum class blend_function : std::uint8_t {
		// result = (source color * source blend) + (destination color * destination blend) 
		add,
		// result = (source color * source blend) − (destination color * destination blend)
		subtract, 
		// result = (destination color * destination blend) − (source color * source blend)
		reverse_subtract, 
		// result = min((source color * source blend), (destination color * destination blend))
		min, 
		// result = max((source color * source blend), (destination color * destination blend))
		max 
	};
	
	struct blend_state {
		static constexpr blend_state addative() {
			blend_state state;
			state.color_source_blend = blend::source_alpha;
			state.alpha_source_blend = blend::source_alpha;
			state.color_destination_blend = blend::one;
			state.alpha_destination_blend = blend::one;
			return state;
		}

		static constexpr blend_state alpha_blend() {
			blend_state state;
			state.color_source_blend = blend::one;
			state.alpha_source_blend = blend::one;
			state.color_destination_blend = blend::inverse_source_alpha;
			state.alpha_destination_blend = blend::inverse_source_alpha;
			return state;
		}

		static constexpr blend_state non_premultiplied() {
			blend_state state;
			state.color_source_blend = blend::source_alpha;
			state.alpha_source_blend = blend::source_alpha;
			state.color_destination_blend = blend::inverse_source_alpha;
			state.alpha_destination_blend = blend::inverse_source_alpha;
			return state;
		}

		static constexpr blend_state opaque() {
			blend_state state;
			state.color_source_blend = blend::one;
			state.alpha_source_blend = blend::one;
			state.color_destination_blend = blend::zero;
			state.alpha_destination_blend = blend::zero;
			return state;
		}

		constexpr blend_state() = default;

		constexpr blend_state(const blend_state&) = default;

		constexpr blend_state(blend_state&&) = default;

		constexpr blend_state& operator=(const blend_state&) = default;

		constexpr blend_state& operator=(blend_state&&) = default;

		constexpr bool operator==(const blend_state& blend_state) const {
			return alpha_blend_function == blend_state.alpha_blend_function &&
				alpha_source_blend == blend_state.alpha_source_blend &&
				alpha_destination_blend == blend_state.alpha_destination_blend &&
				color_blend_function == blend_state.color_blend_function &&
				color_source_blend == blend_state.color_source_blend &&
				color_destination_blend == blend_state.color_destination_blend &&
				color_factor == blend_state.color_factor;
		}

		constexpr bool operator!=(const blend_state& blend_state) const {
			return !operator==(blend_state);
		}

		/**
		 * @brief Returns unique hashcode for blendstate.
		 */
		constexpr uint64_t hash_code() const {
			return (uint64_t)color_factor.r |
				((uint64_t)color_factor.g << 8) |
				((uint64_t)color_factor.b << 16) |
				((uint64_t)color_factor.a << 24) |
				(((uint64_t)(alpha_blend_function) & 0xF) << 32) |
				(((uint64_t)(alpha_destination_blend) & 0xF) << 36) |
				(((uint64_t)(alpha_source_blend) & 0xF) << 40) |
				(((uint64_t)(color_blend_function) & 0xF) << 44) |
				(((uint64_t)(color_destination_blend) & 0xF) << 48) |
				(((uint64_t)(color_source_blend) & 0xF) << 52);
		}

		blend_function alpha_blend_function{ blend_function::add };
		blend alpha_destination_blend{ blend::one };
		blend alpha_source_blend{ blend::one };
		blend_function color_blend_function{ blend_function::add };
		blend color_destination_blend{ blend::one };
		blend color_source_blend{ blend::one };
		color color_factor{ 255, 255, 255, 255 };
		// todo: add color write channels
	};
}
