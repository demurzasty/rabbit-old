#pragma once 

#include "json.hpp"

#include <memory>
#include <string>
#include <typeinfo>
#include <typeindex>
#include <functional>
#include <unordered_map>

namespace rb {
	class assets {
		using loader = std::function<std::shared_ptr<void>(const std::string&, json&)>;

	public:
		static void init();

		static void release();

		template<typename Asset, typename Loader>
		static void add_loader(Loader loader) {
			_loaders.emplace(typeid(Asset), loader);
		}

		template<typename Asset>
		static std::shared_ptr<Asset> load(const std::string& filename) {
			return std::static_pointer_cast<Asset>(_load(typeid(Asset), filename));
		}

	private:
		static std::shared_ptr<void> _load(std::type_index type_index, const std::string& filename);

		static json _load_metadata(const std::string& filename);

	private:
		static std::unordered_map<std::type_index, loader> _loaders;
		static std::unordered_map<std::string, std::weak_ptr<void>> _assets;
	};
}
