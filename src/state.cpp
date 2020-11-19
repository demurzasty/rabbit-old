#include <rabbit/state.hpp>

using namespace rb;

void state::initialize() {
}

void state::release() {
}

void state::update(float elapsed_time) {
}

void state::fixed_update(float fixed_time) {
}

void state::draw() {
}

state_manager* state::state_manager() const {
    return _state_manager;
}
