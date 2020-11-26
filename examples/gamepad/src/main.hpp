#pragma once 

#include <rabbit/rabbit.hpp>

class example_game : public rb::game {
public:
    example_game(rb::config& config);

protected:
    void initialize() override;

    void update(float elapsed_time) override;

    void draw() override;

private:
    std::shared_ptr<rb::sprite_batch> _sprite_batch;
    std::shared_ptr<rb::texture> _texture;
    rb::vec2f _troll_position = { 610.0f, 330.0f };
};
