#pragma once 

#include <memory>
#include <string_view>

namespace rb {
    class loader {
    public:
        virtual ~loader() = default;

        virtual std::shared_ptr<void> load(const std::string_view& filename) = 0;
    };
}
