#pragma once 

#include "loader.hpp"
#include "container.hpp"
#include "type_info.hpp"

#include <memory>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>

namespace rb {
    class asset_manager {
    public:
        asset_manager(container& container);

        template<typename Asset, typename Loader>
        void add_loader() {
            _loaders.emplace(type_id<Asset>().hash(), _container.resolve<Loader>());
        }

        template<typename Asset>
        std::shared_ptr<Asset> load(const std::string& filename) {
            auto& asset = _assets[filename];
            if (!asset.expired()) {
                return std::static_pointer_cast<Asset>(asset.lock());
            }

            auto loaded_asset = load(type_id<Asset>().hash(), filename);
            asset = loaded_asset;
            return std::static_pointer_cast<Asset>(loaded_asset);
        }

        std::string filename(std::shared_ptr<void> asset) const;

    private:
        std::shared_ptr<void> load(id_type asset_id, const std::string& filename);

    private:
        container& _container;
        std::unordered_map<id_type, std::shared_ptr<loader>> _loaders;
        std::unordered_map<std::string, std::weak_ptr<void>> _assets;
    };
}
