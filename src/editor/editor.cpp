#include <rabbit/rabbit.hpp>

#include <algorithm>
#include <fstream>
#include <execution>
#include <filesystem>
#include <chrono>
#include <mutex>

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

	const auto di = std::filesystem::recursive_directory_iterator{ "data" };

	std::vector<std::filesystem::directory_entry> entries;
	std::transform(std::filesystem::begin(di), std::filesystem::end(di), std::back_inserter(entries), [](auto& entry) {
		return entry;
	});

	std::mutex resources_mutex;
	auto resources_json = json::object();

	std::for_each(std::execution::par_unseq, entries.begin(), entries.end(), [&](const auto& dir_entry) {
	//for (const auto& dir_entry : std::filesystem::recursive_directory_iterator{ "data" }) {
		if (!dir_entry.is_regular_file()) {
			return;
		}

		const auto path = dir_entry.path();
		if (!path.has_extension()) {
			return;
		}

		const auto extension = path.extension();
		if (extension == ".meta") {
			return;
		}

		const auto importer = _importers[extension.string()];
		if (!importer) {
			return;
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
		const auto output_path = package_directory / uuid;

		if (last_write_time > cached_last_write_time || !std::filesystem::exists(output_path)) {
			print("importing: {}\n", path.string());

			fibstream input{ path.string() };
			fobstream output{ output_path.string() };

			importer(input, output, metadata);

			cache["last_write_time"] = last_write_time;
			std::ofstream{ cache_path } << std::setw(4) << cache;
		}

		std::lock_guard<std::mutex> guard{ resources_mutex };
		auto resource_path = path.string();
		std::replace(resource_path.begin(), resource_path.end(), '\\', '/');
		resources_json[resource_path] = uuid;
	});

	fobstream resources_stream{ (package_directory / "resources").string() };
	const auto cbor = json::to_cbor(resources_json);
	resources_stream.write<std::uint32_t>(cbor.size());
	resources_stream.write<std::uint8_t>(cbor);
}
