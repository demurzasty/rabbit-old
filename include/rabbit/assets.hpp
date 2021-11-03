#pragma once 

#include "bstream.hpp"
#include "json.hpp"
#include "uuid.hpp"
#include "fnv1a.hpp"

#include <memory>
#include <string>
#include <typeinfo>
#include <typeindex>
#include <functional>
#include <unordered_map>

namespace rb {
	template<typename T>
	class asset {
	public:
		asset() = default;

		asset(const std::shared_ptr<T>& ptr)
			: _ptr(ptr) {
		}

		asset(const asset<T>&) = default;
		asset(asset<T>&&) = default;

		asset<T>& operator=(const asset<T>&) = default;
		asset<T>& operator=(asset<T>&&) = default;

		T* operator->() const {
			return _ptr.get();
		}

	private:
		std::shared_ptr<T> _ptr;
	};

	class assets {
		using loader = std::function<std::shared_ptr<void>(bstream&)>;

	public:
		static void init();

		static void release();

		static void load_resources();

		template<typename Asset, typename Loader>
		static void add_loader(Loader loader) {
			_loaders.emplace(typeid(Asset), loader);
		}

		template<typename Asset>
		static std::shared_ptr<Asset> load(const uuid& uuid) {
			return std::static_pointer_cast<Asset>(_load(typeid(Asset), uuid, Asset::magic_number));
		}

		template<typename Asset>
		static std::shared_ptr<Asset> load(const std::string& name) {
			if (const auto uuid = _resources[name]; uuid) {
				return load<Asset>(uuid);
			} else {
				return nullptr;
			}
		}

		static uuid get_uuid(const std::shared_ptr<void>& asset);

	private:
		static std::shared_ptr<void> _load(std::type_index type_index, const uuid& uuid, fnv1a_result_t magic_number);

	private:
		static std::unordered_map<std::type_index, loader> _loaders;
		static std::unordered_map<uuid, std::weak_ptr<void>, uuid::hasher> _assets;
		static std::unordered_map<std::string, uuid> _resources;
	};
}
