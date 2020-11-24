#pragma once 

#include <rabbit/json_fwd.hpp>

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
        virtual std::shared_ptr<void> load(const std::string& filename, const json& metadata) = 0;

        rb::asset_manager* asset_manager() const;

    private:
        rb::asset_manager* _asset_manager = nullptr;
    };
}
