#pragma once 

#include "config.hpp"
#include "span.hpp"
#include "uuid.hpp"

#include <string>
#include <cstdio>
#include <cstring>
#include <vector>
#include <type_traits>

namespace rb {
	enum class bstream_mode {
		read,
		write
	};

	class bstream {
	public:
		bstream(bstream_mode mode)
			: _mode(mode) {
		}

		bstream(const bstream&) = delete;
		bstream(bstream&&) = delete;

		bstream& operator=(const bstream&) = delete;
		bstream& operator=(bstream&&) = delete;

		virtual ~bstream() = default;

		virtual std::size_t size() = 0;

		virtual void write(const void* data, std::size_t size) = 0;

		virtual void read(void* data, std::size_t size) = 0;

		void write(const uuid& uuid) {
			write(uuid.data().data(), uuid.data().size_bytes());
		}

		void read(uuid & uuid) {
			std::uint8_t data[16];
			read(data, sizeof(data));
			uuid = { data };
		}

		template<typename T>
		void write(const span<const T>& data) {
			write(data.data(), data.size_bytes());
		}

		template<typename T>
		void read(const span<T>& data) {
			read(data.data(), data.size_bytes());
		}

		template<typename T, std::enable_if_t<std::is_standard_layout_v<T>&& std::is_trivial_v<T>, int> = 0>
		void write(const T& data) {
			write(&data, sizeof(T));
		}

		template<typename T, std::enable_if_t<std::is_standard_layout_v<T>&& std::is_trivial_v<T>, int> = 0>
		void read(T& data) {
			read(&data, sizeof(T));
		}

		bstream_mode mode() const {
			return _mode;
		}

	private:
		bstream_mode _mode;
	};

	class memory_bstream : public bstream {
	public:
		memory_bstream(const span<const std::uint8_t>& data, bstream_mode mode)
			: bstream(mode)
			, _memory(data.begin(), data.end()) {
		}

		std::size_t size() override {
			return _memory.size();
		}

		void write(const void* data, std::size_t size) override {
			if (_position + size > _memory.size()) {
				_memory.resize(_position + size);
			}

			std::memcpy(&_memory[_position], data, size);
			_position += size;
		}

		void read(void* data, std::size_t size) override {
			RB_ASSERT(_position + size < _memory.size(), "Overflow");

			std::memcpy(data, &_memory[_position], size);
			_position += size;
		}

		template<typename T>
		void write(const span<const T>& data) {
			write(data.data(), data.size_bytes());
		}

		template<typename T>
		void read(const span<T>& data) {
			read(data.data(), data.size_bytes());
		}

		template<typename T, std::enable_if_t<std::is_standard_layout_v<T>&& std::is_trivial_v<T>, int> = 0>
		void write(const T& data) {
			write(&data, sizeof(T));
		}

		template<typename T, std::enable_if_t<std::is_standard_layout_v<T>&& std::is_trivial_v<T>, int> = 0>
		void read(T& data) {
			read(&data, sizeof(T));
		}

	private:
		std::size_t _position{ 0 };
		std::vector<std::uint8_t> _memory;
	};

	class file_bstream : public bstream {
	public:
		file_bstream(const std::string& filename, bstream_mode mode)
			: bstream(mode)
			, _file(fopen(filename.c_str(), mode == bstream_mode::read ? "rb" : "wb"))
			, _filename(filename) {
		}

		~file_bstream() {
			fclose(_file);
		}

		std::size_t size() override {
			const auto curr = ftell(_file);

			fseek(_file, 0, SEEK_END);
			const auto size = ftell(_file);
			fseek(_file, curr, SEEK_SET);

			return size;
		}

		void write(const void* data, std::size_t size) override {
			fwrite(data, 1, size, _file);
		}

		void read(void* data, std::size_t size) override {
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

		template<typename T, std::enable_if_t<std::is_standard_layout_v<T>&& std::is_trivial_v<T>, int> = 0>
		void write(const T& data) {
			write(&data, sizeof(T));
		}

		template<typename T, std::enable_if_t<std::is_standard_layout_v<T>&& std::is_trivial_v<T>, int> = 0>
		void read(T& data) {
			read(&data, sizeof(T));
		}

		const std::string& filename() const {
			return _filename;
		}

	private:
		FILE* _file;
		std::string _filename;
	};
}
