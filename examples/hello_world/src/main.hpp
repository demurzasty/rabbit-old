#pragma once 

#include <rabbit/rabbit.hpp>

class example_game : public rb::game {
public:
    example_game(std::shared_ptr<rb::config> config);

protected:
    void initialize() override;

    void update(float elapsed_time) override;

    void draw() override;
};
