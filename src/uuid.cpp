#include <rabbit/uuid.hpp>

#include <cstring>
#include <cstdio>
#include <random>

using namespace rb;

static std::random_device random;
static std::mt19937 generator{ random() };
static std::uniform_int_distribution<std::uint32_t> distribution{ 0, std::numeric_limits<std::uint32_t>::max() };
static std::uint32_t empty_uuid[4] = { 0, 0, 0, 0 };

static std::uint8_t hex_to_uint8(const char ch) {
	if (ch >= '0' && ch <= '9') {
		return ch - '0';
	} else if (ch >= 'a' && ch <= 'f') {
		return 10 + ch - 'a';
	} else if (ch >= 'A' && ch <= 'F') {
		return 10 + ch - 'A';
	}
	return 0;
}

static std::uint8_t hex_pair_to_uint8(const char a, const char b) {
	return (hex_to_uint8(a) << 4) | hex_to_uint8(b);
}

static bool is_hex(const char ch) {
	return (ch >= '0' && ch <= '9') ||
		(ch >= 'a' && ch <= 'f') ||
		(ch >= 'A' && ch <= 'F');
}

uuid uuid::generate() {
	std::uint32_t data[4];
	data[0] = distribution(generator);
	data[1] = distribution(generator);
	data[2] = distribution(generator);
	data[3] = distribution(generator);
	return data;
}

std::optional<uuid> uuid::from_string(const std::string& string) {
	uuid uuid;

	auto firstDigit = true;
	auto digit = '0';
	auto index = 0;

	for (const auto character : string) {
		if (character == '-') {
			continue;
		}

		if (index >= 16 || !is_hex(character)) {
			return {};
		}

		if (firstDigit) {
			digit = character;
		} else {
			uuid._data[index++] = hex_pair_to_uint8(digit, character);;
		}

		firstDigit = !firstDigit;
	}

	if (index < 16) {
		return {};
	}

	return uuid;
}

uuid::uuid()
	: uuid(empty_uuid) {
}

uuid::uuid(const std::uint8_t data[16]) {
	std::memcpy(_data, data, sizeof(_data));
}

uuid::uuid(const std::uint16_t data[8]) {
	std::memcpy(_data, data, sizeof(_data));
}

uuid::uuid(const std::uint32_t data[4]) {
	std::memcpy(_data, data, sizeof(_data));
}

bool uuid::operator==(const uuid& uuid) const {
	return memcmp(_data, uuid._data, sizeof(_data)) == 0;
}

bool uuid::operator!=(const uuid& uuid) const {
	return memcmp(_data, uuid._data, sizeof(_data)) != 0;
}

bool uuid::operator<(const uuid& uuid) const {
	return memcmp(_data, uuid._data, sizeof(_data)) < 0;
}

bool uuid::operator<=(const uuid& uuid) const {
	return memcmp(_data, uuid._data, sizeof(_data)) <= 0;
}

bool uuid::operator>(const uuid& uuid) const {
	return memcmp(_data, uuid._data, sizeof(_data)) > 0;
}

bool uuid::operator>=(const uuid& uuid) const {
	return memcmp(_data, uuid._data, sizeof(_data)) >= 0;
}

uuid::operator bool() const {
	return memcmp(_data, empty_uuid, sizeof(_data)) != 0;
}

std::string uuid::to_string() const {
	char buffer[64];
	std::sprintf(buffer,
		"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		_data[0], _data[1], _data[2], _data[3],
		_data[4], _data[5],
		_data[6], _data[7],
		_data[8], _data[9],
		_data[10], _data[11], _data[12], _data[13], _data[14], _data[15]);
	return buffer;
}
