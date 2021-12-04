#pragma once 

#include "uuid.hpp"
#include "type.hpp"
#include "loader.hpp"
#include "reactive.hpp"

#include <memory>
#include <unordered_map>

namespace rb {
    class asset_storage {
    public:
        asset_storage(const std::shared_ptr<void>& ptr);

        asset_storage(const asset_storage&) = delete;
        asset_storage(asset_storage&&) = default;

        asset_storage& operator=(const asset_storage&) = delete;
        asset_storage& operator=(asset_storage&&) = default;

        void reload(const std::shared_ptr<void>& ptr);

        const std::shared_ptr<void>& ptr() const noexcept;

        template<typename T>
        std::shared_ptr<T> ptr() const {
            return std::static_pointer_cast<T>(_ptr);
        }

        sink<void()> on_reload() noexcept;

    private:
        std::shared_ptr<void> _ptr;
        sigh<void()> _reload;
    };

    template<typename T>
    class asset {
    public:
        asset() = default;

        asset(const std::shared_ptr<asset_storage>& storage)
            : _storage(storage) {
        }

        asset(const std::shared_ptr<T>& ptr)
            : _storage(std::make_shared<asset_storage>(ptr)) {
        }

        asset(const asset<T>&) = default;
        asset(asset<T>&&) = default;

        asset<T>& operator=(const asset<T>&) = default;
        asset<T>& operator=(asset<T>&&) = default;

        T* operator->() const noexcept {
            return _storage->ptr<T>().get();
        }

        operator bool() const noexcept {
            return _storage;
        }

        bool valid() const noexcept {
            return _storage;
        }

        asset_storage& storage() {
            return *_storage;
        }

    private:
        std::shared_ptr<asset_storage> _storage;
    };

    template<typename T, typename... Args>
    asset<T> make_asset(Args&&... args) {
        return { std::make_shared<T>(std::forward<Args>(args)...) };
    }

    class assets_impl {
    public:
        template<typename Asset, typename Loader, typename... Args>
        void add_loader(Args&&... args) {
            _loaders.emplace(type_id<Asset>().hash(), std::make_shared<Loader>(std::forward<Args>(args)...));
        }

        template<typename T>
        asset<T> load(const uuid& uuid) {
            return _load(type_id<T>().hash(), uuid);
        }

    private:
        std::shared_ptr<asset_storage> _load(id_type asset_type_id, const uuid& uuid);

    private:
        std::unordered_map<id_type, std::shared_ptr<loader>> _loaders;
        std::unordered_map<uuid, std::weak_ptr<asset_storage>, uuid::hasher> _assets;
    };

    class assets {
    public:
        static void setup();

        static void release();

        template<typename Asset, typename Loader, typename... Args>
        static void add_loader(Args&&... args) {
            _impl->add_loader<Asset, Loader>(std::forward<Args>(args)...);
        }

        template<typename T>
        static asset<T> load(const uuid& uuid) {
            return _impl->load<T>(uuid);
        }

    private:
        static std::unique_ptr<assets_impl> _impl;
    };
}
