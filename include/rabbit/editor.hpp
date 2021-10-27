#pragma once 

#include "json.hpp"

#include <string>
#include <functional>
#include <unordered_map>
#include <filesystem>

namespace rb {
	class entry {
		std::filesystem::path path;
	};

	using importer = std::function<void(const std::string&, const std::string&, const json&)>;

	class editor {
	public:
		static void init();

		static void release();

		static void scan();

		template<typename Asset, typename... Extensions>
		static void add_importer(Extensions&&... extensions) {
			(_importers.emplace(extensions, &Asset::import), ...);
		}

	private:
		static std::unordered_map<std::string, importer> _importers;
	};
}
