#pragma once 

namespace rb {
	template<typename T>
	void safe_release(T*& ppt) {
		if (ppt) {
			ppt->Release();
			ppt = nullptr;
		}
	}
}
