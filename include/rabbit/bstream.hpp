#pragma once 

#include "config.hpp"
#include "span.hpp"

#include <string>
#include <cstdio>
#include <type_traits>

namespace rb {
	enum class bstream_mode {
		read,
		write
	};

	class bstream {
	public:
		bstream(const std::string& filename, bstream_mode mode)
			: _file(fopen(filename.c_str(), mode == bstream_mode::read ? "rb" : "wb")) {
		}

		bstream(const bstream&) = delete;
		bstream(bstream&&) = default;

		bstream& operator=(const bstream&) = delete;
		bstream& operator=(bstream&&) = default;

		~bstream() {
			fclose(_file);
		}

		void write(const void* data, std::size_t size) {
			fwrite(data, 1, size, _file);
		}

		void read(void* data, std::size_t size) {
			fread(data, 1, size, _file);
		}

		template<typename T>
		void write(const span<const T>& data) {
			write(data.data(), data.size_bytes());
		}


		template<typename T>
		void read(const span<T>& data) {
			read(data.data(), data.size_bytes());
		}

		template<typename T, std::enable_if_t<std::is_standard_layout_v<T> && std::is_trivial_v<T>, int> = 0>
		void write(const T& data) {
			write(&data, sizeof(T));
		}

		template<typename T, std::enable_if_t<std::is_standard_layout_v<T>&& std::is_trivial_v<T>, int> = 0>
		void read(T& data) {
			read(&data, sizeof(T));
		}

	private:
		FILE* _file;
	};
}
