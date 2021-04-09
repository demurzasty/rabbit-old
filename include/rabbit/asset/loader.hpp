#pragma once 

#include "../core/json_fwd.hpp"

#include <string>
#include <memory>

namespace rb {
    struct loader {
        virtual ~loader() = default;

        /**
         * @brief
         */
        virtual std::shared_ptr<void> load(const std::string& filename, const json& metadata) = 0;
    };
}
