#pragma once 

#include "json.hpp"

#include <string>
#include <memory>

namespace rb {
    class loader {
    public:
        virtual ~loader() = default;

        virtual std::shared_ptr<void> load(const std::string& filename, const json& json) = 0;
    };
}
