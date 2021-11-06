#include <rabbit/rabbit.hpp>

#include <filesystem>

using namespace rb;

struct camera_controller : public rb::system {
    void update(registry& registry, float elapsed_time) {
        if (input::is_mouse_button_pressed(mouse_button::right)) {
            _last_mouse_position = input::mouse_position();
        }

        registry.view<transform, camera>().each([this, elapsed_time](transform& transform, camera& camera) {
            if (input::is_mouse_button_down(mouse_button::right)) {
                const auto speed = input::is_key_down(keycode::space) ? 10.0f : 1.0f;

                const auto mouse_position = input::mouse_position();
                const auto diff = _last_mouse_position - mouse_position;
                _last_mouse_position = mouse_position;

                transform.rotation.y += diff.x * 0.005f;
                transform.rotation.x += diff.y * 0.005f;

                if (input::is_key_down(keycode::e)) {
                    transform.position.y += elapsed_time * speed;
                }
                else if (input::is_key_down(keycode::q)) {
                    transform.position.y -= elapsed_time * speed;
                }

                if (input::is_key_down(keycode::w)) {
                    transform.position.x -= std::sin(transform.rotation.y) * elapsed_time * speed;
                    transform.position.z -= std::cos(transform.rotation.y) * elapsed_time * speed;
                }
                else if (input::is_key_down(keycode::s)) {
                    transform.position.x += std::sin(transform.rotation.y) * elapsed_time * speed;
                    transform.position.z += std::cos(transform.rotation.y) * elapsed_time * speed;
                }

                if (input::is_key_down(keycode::d)) {
                    transform.position.x -= std::sin(transform.rotation.y - pi<float>() * 0.5f) * elapsed_time * speed;
                    transform.position.z -= std::cos(transform.rotation.y - pi<float>() * 0.5f) * elapsed_time * speed;
                }
                else if (input::is_key_down(keycode::a)) {
                    transform.position.x -= std::sin(transform.rotation.y + pi<float>() * 0.5f) * elapsed_time * speed;
                    transform.position.z -= std::cos(transform.rotation.y + pi<float>() * 0.5f) * elapsed_time * speed;
                }
            }
        });
    }
private:
    vec2i _last_mouse_position;
};

class fps_meter : public rb::system {
public:
    void update(registry& registry, float elapsed_time) override {
        _time += elapsed_time;
        if (_time > 1.0f) {
            window::set_title(format("RabBit FPS: {}", _fps));
            _fps = 0;
           // _time = std::modf(_time, 1.0f);
            _time -= 1.0f;
        }
    }

    void draw(registry& registry) override {
        ++_fps;
    }

private:
    int _fps{ 0 };
    float _time{ 0.0f };
};

int main(int argc, char* argv[]) {
#if !RB_PROD_BUILD
    std::filesystem::current_path(CURRENT_DIRECTORY);
#endif

    app::setup();

    app::system<camera_controller>();
    app::system<fps_meter>();

    app::run("data/prefabs/scene.scn");
}
