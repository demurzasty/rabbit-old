#pragma once 

#include <rabbit/state.hpp>

class test_state : public rb::state {
public:
    void update(float elapsed_time) override;
};