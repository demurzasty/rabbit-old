#pragma once 

#include "span.hpp"

#include <vector>

namespace rb {
	class compression {
	public:
		static std::size_t compress_bound(std::size_t uncompressed_size);

		static std::size_t compress(const void* uncompressed_data, std::size_t uncompressed_size, void* compressed_data, std::size_t compressed_bound);

		static std::size_t uncompress(const void* compressed_data, std::size_t compressed_size, void* uncompressed_data, std::size_t uncompressed_size);

		template<typename T>
		static std::vector<std::uint8_t> compress(const span<const T>& uncompressed_data) {
			std::vector<std::uint8_t> compressed_data(compress_bound(uncompressed_data.size_bytes()));
			const auto compressed_size = compress(uncompressed_data.data(), uncompressed_data.size_bytes(), compressed_data.data(), compressed_data.size());
			return compressed_size > 0 ? compressed_data : std::vector<std::uint8_t>{};
		}

		template<typename T>
		static std::vector<T> uncompress(std::size_t uncompressed_size, const span<const std::uint8_t>& compressed_data) {
			std::vector<T> uncompressed_data(uncompressed_size / sizeof(T));
			const auto size = uncompress(compressed_data.data(), compressed_data.size_bytes(), uncompressed_data.data(), uncompressed_size);
			return size > 0 ? uncompressed_data : std::vector<T>{};
		}
	};
}
