#include <rabbit/loader.hpp>

using namespace rb;

asset_manager* loader::asset_manager() const {
    return _asset_manager;
}
