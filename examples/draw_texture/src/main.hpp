#pragma once 

#include <rabbit/rabbit.hpp>

class draw_texture : public rb::game {
public:
    draw_texture(rb::config& config);

protected:
    void initialize() override;

    void update(float elapsed_time) override;

    void draw() override;

private:
    std::shared_ptr<rb::texture> _texture;
};
