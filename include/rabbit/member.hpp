#pragma once 

namespace rb {
	template<typename Owner, typename Data>
	class member {
	public:
		member(Owner* owner, Data data = {})
			: _owner(owner)
			, _data(data) {
		}

		member& operator=(const Data& data) {
			if (_data != data) {
				_data = data;
				_owner->mark_dirty();
			}
			return *this;
		}

		operator Data() const {
			return _data;
		}

	private:
		Owner* _owner;
		Data _data;
	};
}
