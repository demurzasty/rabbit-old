#include <rabbit/rabbit.hpp>

#include <fstream>

using namespace rb;

std::unordered_map<std::string, importer> editor::_importers;

void editor::init() {
	add_importer<texture>(".png", ".bmp", ".jpg");
	add_importer<mesh>(".obj");
	add_importer<material>(".mat");
	add_importer<environment>(".env");
	add_importer<prefab>(".scn");
}

void editor::release() {
	_importers.clear();
}

void editor::scan() {
	const auto package_directory = std::filesystem::current_path() / "package";
	const auto cache_directory = std::filesystem::current_path() / "cache";

	std::filesystem::create_directories(package_directory);
	std::filesystem::create_directories(cache_directory);

	for (auto& dir_entry : std::filesystem::recursive_directory_iterator{ "./data" }) {
		if (!dir_entry.is_regular_file()) {
			continue;
		}

		const auto path = dir_entry.path();
		if (!path.has_extension()) {
			continue;
		}

		const auto extension = path.extension();
		if (extension == ".meta") {
			continue;
		}

		const auto importer = _importers[extension.string()];
		if (!importer) {
			continue;
		}

		const auto meta_path = path.string() + ".meta";

		json metadata;
		if (std::filesystem::exists(meta_path)) {
			std::ifstream{ meta_path } >> metadata;
		} else {
			metadata["uuid"] = uuid::generate().to_string();
			std::ofstream{ meta_path } << std::setw(4) << metadata;
		}

		const std::string uuid{ metadata["uuid"] };

		const auto cache_path = cache_directory / uuid;
		const long long last_write_time = dir_entry.last_write_time().time_since_epoch().count();

		json cache;
		if (std::filesystem::exists(cache_path)) {
			std::ifstream{ cache_path } >> cache;
		} else {
			cache["last_write_time"] = 0ll;
			std::ofstream{ cache_path } << std::setw(4) << cache;
		}

		const long long cached_last_write_time{ cache["last_write_time"] };

		if (last_write_time > cached_last_write_time) {
			print("importing: {}\n", path.string());

			const auto output_path = package_directory / uuid;
			importer(path.string(), output_path.string(), metadata);

			cache["last_write_time"] = last_write_time;
			std::ofstream{ cache_path } << std::setw(4) << cache;
		}
	}
}
