#pragma once 

#include <string>
#include <memory>

namespace rb {
    class loader {
    public:
        virtual ~loader() = default;
            
        /**
         * @brief 
         */
        virtual std::shared_ptr<void> load(const std::string& filename) = 0;
    };
}