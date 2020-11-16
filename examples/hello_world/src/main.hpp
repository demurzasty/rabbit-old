#pragma once 

#include <rabbit/rabbit.hpp>

class hello_world : public rb::game {
public:
    hello_world(rb::config& config);

protected:
    void initialize() override;

    void update(float elapsed_time) override;

    void draw() override;
};
