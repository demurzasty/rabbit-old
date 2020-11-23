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
    std::shared_ptr<rb::texture> _texture;
    std::shared_ptr<rb::buffer> _vertex_buffer;
    std::shared_ptr<rb::buffer> _index_buffer;
    float _rotation = 0.0f;
};
