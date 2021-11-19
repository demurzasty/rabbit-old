#include <rabbit/engine/core/compression.hpp>

#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include <miniz.h>

using namespace rb;

std::size_t compression::compress_bound(std::size_t uncompressed_size) {
	return mz_compressBound(uncompressed_size);
}

std::size_t compression::compress(const void* uncompressed_data, std::size_t uncompressed_size, void* compressed_data, std::size_t compressed_bound) {
	mz_ulong compressed_size = (mz_ulong)compressed_bound;
	
	const auto status = mz_compress2((unsigned char*)compressed_data,
		&compressed_size,
		(const unsigned char*)uncompressed_data,
		uncompressed_size,
		1);

	return status == MZ_OK ? compressed_size : 0;
}

std::size_t compression::uncompress(const void* compressed_data, std::size_t compressed_size, void* uncompressed_data, std::size_t uncompressed_size) {
	mz_ulong uncompressed_size2 = (mz_ulong)uncompressed_size;

	const auto status = mz_uncompress((unsigned char*)uncompressed_data,
		&uncompressed_size2,
		(const unsigned char*)compressed_data,
		compressed_size);

	return status == MZ_OK ? uncompressed_size2 : 0;
}