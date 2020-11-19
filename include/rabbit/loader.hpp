#pragma once 

#include <string>
#include <memory>

namespace rb {
    class asset_manager;

    class loader {
        friend class asset_manager;

    public:
        virtual ~loader() = default;

        /**
         * @brief
         */
        virtual std::shared_ptr<void> load(const std::string& filename) = 0;

        asset_manager* asset_manager() const;

    private:
        rb::asset_manager* _asset_manager = nullptr;
    };
}
