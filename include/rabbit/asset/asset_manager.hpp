#pragma once 

#include "loader.hpp"
#include "../core/container.hpp"
#include "../core/type_info.hpp"

#include <memory>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>

namespace rb {
    /**
     * @brief Loads resources from the binary files. It also manages the lifespan of the loaded objects.
     */
    class asset_manager {
    public:
        /**
         * @brief Constructor. Need dependency container for loaders resolving.
         * 
         * @param container Dependency container for loaders resolving.
         */
        asset_manager(container& container);

        /**
         * @brief Register loader using asset type. Loader will be automaticly resolved using dependency injector.
         * 
         * @tparam Asset Asset type to register for.
         * @tparam Loader Loader type to resolve.
         */
        template<typename Asset, typename Loader>
        void add_loader() {
            _loaders.emplace(type_id<Asset>().hash(), _container.resolve<Loader>());
        }

        /**
         * @brief Loads a new asset using filename and registered loader for Asset.
         *        Return reference if asset is already loaded.
         * 
         * @return Loaded asset.
         */
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

        /**
         * @brief Retrieve filename for loaded asset.
         * 
         * @return Asset's filename or empty string if not loaded.
         */
        std::string filename(std::shared_ptr<void> asset) const;

    private:
        std::shared_ptr<void> load(id_type asset_id, const std::string& filename);

    private:
        container& _container;
        std::unordered_map<id_type, std::shared_ptr<loader>> _loaders;
        std::unordered_map<std::string, std::weak_ptr<void>> _assets;
    };
}
