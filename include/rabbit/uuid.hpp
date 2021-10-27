#pragma once 

#include <string>
#include <optional>
#include <cstdint>

namespace rb {
	class uuid {
	public:
		static uuid generate();

		static std::optional<uuid> from_string(const std::string& string);

	public:
		uuid();

		uuid(const std::uint8_t data[16]);

		uuid(const std::uint16_t data[8]);

		uuid(const std::uint32_t data[4]);

		uuid(const uuid&) = default;

		uuid(uuid&&) = default;

		uuid& operator=(const uuid&) = default;

		uuid& operator=(uuid&&) = default;

		bool operator==(const uuid& uuid) const;

		bool operator!=(const uuid& uuid) const;

		bool operator<(const uuid& uuid) const;

		bool operator<=(const uuid& uuid) const;

		bool operator>(const uuid& uuid) const;

		bool operator>=(const uuid& uuid) const;

		operator bool() const;

		std::string to_string() const;

	private:
		std::uint8_t _data[16];
	};
}
