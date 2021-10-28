#pragma once 

#include <cstddef>

namespace rb {
	class compression {
	public:
		static std::size_t compress_bound(std::size_t uncompressed_size);

		static std::size_t compress(const void* uncompressed_data, std::size_t uncompressed_size, void* compressed_data);

		static std::size_t uncompress(const void* compressed_data, std::size_t compressed_size, void* uncompressed_data);
	};
}
