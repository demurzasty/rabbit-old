#pragma once 

#include "bstream.hpp"
#include "json.hpp"
#include "uuid.hpp"

#include <memory>
#include <string>
#include <typeinfo>
#include <typeindex>
#include <functional>
#include <unordered_map>

namespace rb {
	class assets {
		using loader = std::function<std::shared_ptr<void>(bstream&)>;

	public:
		static void init();

		static void release();

		template<typename Asset, typename Loader>
		static void add_loader(Loader loader) {
			_loaders.emplace(typeid(Asset), loader);
		}

		template<typename Asset>
		static std::shared_ptr<Asset> load(const uuid& uuid) {
			return std::static_pointer_cast<Asset>(_load(typeid(Asset), uuid));
		}

		template<typename Asset>
		static std::shared_ptr<Asset> load(const std::string& uuid_str) {
			if (const auto uuid = uuid::from_string(uuid_str); uuid.has_value()) {
				return load<Asset>(uuid.value());
			} else {
				return nullptr;
			}
		}

		static uuid get_uuid(const std::shared_ptr<void>& asset);

	private:
		static std::shared_ptr<void> _load(std::type_index type_index, const uuid& uuid);

	private:
		static std::unordered_map<std::type_index, loader> _loaders;
		static std::unordered_map<uuid, std::weak_ptr<void>, uuid::hasher> _assets;
	};
}
