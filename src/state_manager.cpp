#include <rabbit/state_manager.hpp>

using namespace rb;

void state_manager::add(const std::string& name, std::shared_ptr<state> state) {
    // Inject state manager.
    state->_state_manager = this;

    // Add new state.
    _states.emplace(name, state);
}

void state_manager::set(const std::string& name) {
    if (_current_state) {
        _current_state->release();
    }

    _current_state = _states.at(name);
    _current_state->initialize();
}

void state_manager::initialize() {
    if (_current_state) {
        _current_state->initialize();
    }
}

void state_manager::release() {
    if (_current_state) {
        _current_state->release();
    }
}

void state_manager::update(float elapsed_time) {
    if (_current_state) {
        _current_state->update(elapsed_time);
    }
}

void state_manager::fixed_update(float fixed_time) {
    if (_current_state) {
        _current_state->fixed_update(fixed_time);
    }
}

void state_manager::draw() {
    if (_current_state) {
        _current_state->draw();
    }
}
