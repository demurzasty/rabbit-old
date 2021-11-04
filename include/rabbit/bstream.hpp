#pragma once 

#include "span.hpp"
#include "uuid.hpp"
#include "json.hpp"

#include <ios>
#include <string>
#include <cstdio>
#include <memory>
#include <type_traits>

namespace rb {
	// input binary stream
	class ibstream {
	public:
		ibstream() = default;

		virtual ~ibstream() = default;

		ibstream(const ibstream&) = delete;
		ibstream(ibstream&&) = delete;

		ibstream& operator=(const ibstream&) = delete;
		ibstream& operator=(ibstream&&) = delete;

		virtual void read(void* data, std::streamsize size) = 0;

		virtual void seek(std::streamoff offset) = 0;

		virtual bool eof() = 0;

		virtual std::streamsize size() = 0;

		void read(uuid& uuid);

		void read(json& json);

		template<typename T>
		void read(const span<T>& data) {
			read(data.data(), data.size_bytes());
		}

		template<typename T, std::enable_if_t<std::is_standard_layout_v<T>&& std::is_trivial_v<T>, int> = 0>
		void read(T& data) {
			read(&data, sizeof(T));
		}
	};

	// output binary stream 
	class obstream {
	public:
		obstream() = default;

		virtual ~obstream() = default;

		obstream(const obstream&) = delete;
		obstream(obstream&&) = delete;

		obstream& operator=(const obstream&) = delete;
		obstream& operator=(obstream&&) = delete;

		virtual void write(const void* data, std::streamsize size) = 0;

		void write(const uuid& uuid);

		template<typename T>
		void write(const span<const T>& data) {
			write(data.data(), data.size_bytes());
		}

		template<typename T, std::enable_if_t<std::is_standard_layout_v<T>&& std::is_trivial_v<T>, int> = 0>
		void write(const T& data) {
			write(&data, sizeof(T));
		}
	};

	// file input binary stream 
	class fibstream : public ibstream {
	public:
		fibstream(const std::string& filename);

		virtual ~fibstream() = default;

		virtual void read(void* data, std::streamsize size) override;

		virtual void seek(std::streamoff offset) override;

		virtual bool eof() override;

		virtual std::streamsize size() override;

		const std::string& filename() const;

		using ibstream::read;

	private:
		std::string _filename;
		std::shared_ptr<std::ifstream> _stream;
	};

	// file output binary stream
	class fobstream : public obstream {
	public:
		fobstream(const std::string& filename);

		virtual ~fobstream() = default;

		virtual void write(const void* data, std::streamsize size) override;

		const std::string& filename() const;

		using obstream::write;

	private:
		std::string _filename;
		std::shared_ptr<std::ofstream> _stream;
	};

	// memory input binary stream
	class mibstream : public ibstream {
	public:
		mibstream(const span<const std::uint8_t>& memory);

		virtual ~mibstream() = default;

		virtual void read(void* data, std::streamsize size) override;

		virtual void seek(std::streamoff offset) override;

		virtual bool eof() override;

		virtual std::streamsize size() override;

		const span<const std::uint8_t> memory() const;

		using ibstream::read;

	private:
		std::streampos _position{ 0 };
		std::vector<std::uint8_t> _memory;
	};

	// memory output binary stream
	class mobstream : public obstream {
	public:
		mobstream() = default;

		virtual ~mobstream() = default;

		virtual void write(const void* data, std::streamsize size) override;

		const span<const std::uint8_t> memory() const;

		using obstream::write;

	private:
		std::streampos _position{ 0 };
		std::vector<std::uint8_t> _memory;
	};
}
