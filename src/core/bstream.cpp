#include <rabbit/core/bstream.hpp>
#include <rabbit/core/config.hpp>

#include <cstring>
#include <fstream>

using namespace rb;

void obstream::write(const uuid& uuid) {
	write(uuid.data());
}

void ibstream::read(uuid& uuid) {
	std::uint8_t data[16];
	read(data, sizeof(data));
	uuid = { data };
}

void ibstream::read(json& json) {
	const auto buffer_size = size();

	std::string str;
	str.resize(buffer_size);
	read(&str[0], buffer_size);

	json = json::parse(str);
}

fibstream::fibstream(const std::string& filename)
	: _filename(filename) {
	_stream = std::make_shared<std::ifstream>(filename, std::ios::binary);
}

void fibstream::read(void* data, std::streamsize size) {
	_stream->read(reinterpret_cast<char*>(data), size);
}

void fibstream::seek(std::streamoff offset) {
	_stream->seekg(offset, std::ios_base::cur);
}

bool fibstream::eof() {
	return _stream->eof();
}

std::streamsize fibstream::size() {
	const auto pos = _stream->tellg();
	_stream->seekg(0, std::ios_base::end);
	const auto size = _stream->tellg();
	_stream->seekg(pos);
	return size;
}

const std::string& fibstream::filename() const {
	return _filename;
}

fobstream::fobstream(const std::string& filename)
	: _filename(filename) {
	_stream = std::make_shared<std::ofstream>(filename, std::ios::binary);
}

void fobstream::write(const void* data, std::streamsize size) {
	_stream->write(reinterpret_cast<const char*>(data), size);
}

const std::string& fobstream::filename() const {
	return _filename;
}

mibstream::mibstream(const span<const std::uint8_t>& memory)
	: _memory(memory.begin(), memory.end()) {
}

void mibstream::read(void* data, std::streamsize size) {
	RB_ASSERT(_position + size <= _memory.size(), "Overflow");
	std::memcpy(data, &_memory[_position], size);
	_position += size;
}

void mibstream::seek(std::streamoff offset) {
	_position += offset;
}

bool mibstream::eof() {
	return _position == _memory.size();
}

std::streamsize mibstream::size() {
	return _memory.size();
}

const span<const std::uint8_t> mibstream::memory() const {
	return _memory;
}

void mobstream::write(const void* data, std::streamsize size) {
	_memory.resize(_memory.size() + size);
	std::memcpy(&_memory[_position], data, size);
	_position += size;
}

const span<const std::uint8_t> mobstream::memory() const {
	return _memory;
}
