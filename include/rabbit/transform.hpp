#pragma once 

#include "entity.hpp"
#include "vec3.hpp"
#include "member.hpp"

namespace rb {
	class transform {
	public:
		template<typename Visitor>
		static void visit(Visitor& visitor, transform& transform) {
			visitor("position", transform.position);
			visitor("rotation", transform.rotation);
			visitor("scaling", transform.scaling);
		}

		void mark_dirty(bool dirty = true) {
			_dirty = dirty;
		}

		bool dirty() const {
			return _dirty;
		}

	public:
		entity parent{ null };
		member<transform, vec3f> position{ this, { 0.0f, 0.0f, 0.0f } };
		member<transform, vec3f> rotation{ this, { 0.0f, 0.0f, 0.0f } };
		member<transform, vec3f> scaling{ this, { 1.0f, 1.0f, 1.0f } };

	private:
		bool _dirty{ true };
	};
}
