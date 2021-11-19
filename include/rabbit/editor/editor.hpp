#pragma once 

#include <rabbit/engine/core/json.hpp>
#include <rabbit/engine/core/bstream.hpp>

#include <string>
#include <functional>
#include <unordered_map>
#include <filesystem>

namespace rb {
	class entry {
		std::filesystem::path path;
	};

	using importer = std::function<void(ibstream&, obstream&, const json&)>;

	class editor {
	public:
		static void init();

		static void release();

		static void scan();

		static uuid import(const std::string& filename);

		template<typename Asset, typename... Extensions>
		static void add_importer(Extensions&&... extensions) {
			(_importers.emplace(extensions, &Asset::import), ...);
		}

	private:
		static std::unordered_map<std::string, importer> _importers;
	};
}
