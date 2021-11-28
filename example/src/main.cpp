#include <rabbit/rabbit.hpp>

#include <filesystem>

using namespace rb;

struct camera_controller : public rb::system {
    void update(registry& registry, float elapsed_time) override {
        if (input::is_mouse_button_pressed(mouse_button::right)) {
            _last_mouse_position = input::mouse_position();
        }

        for (const auto& [entity, transform, camera] : registry.view<transform, camera>().each()) {
            if (input::is_mouse_button_down(mouse_button::right)) {
                const auto speed = input::is_key_down(keycode::space) ? 10.0f : 2.5f;

                const auto mouse_position = input::mouse_position();
                const auto diff = _last_mouse_position - mouse_position;
                _last_mouse_position = mouse_position;

                registry.patch<rb::transform>(entity, [this, diff, elapsed_time, speed](rb::transform& transform) {
                    _target_camera_rotation.y += diff.x * 0.005f;
                    _target_camera_rotation.x += diff.y * 0.005f;

                    if (input::is_key_down(keycode::e)) {
                        _target_camera_position.y += elapsed_time * speed;
                    } else if (input::is_key_down(keycode::q)) {
                        _target_camera_position.y -= elapsed_time * speed;
                    }

                    if (input::is_key_down(keycode::w)) {
                        _target_camera_position.x -= std::sin(_target_camera_rotation.y) * elapsed_time * speed;
                        _target_camera_position.z -= std::cos(_target_camera_rotation.y) * elapsed_time * speed;
                    } else if (input::is_key_down(keycode::s)) {
                        _target_camera_position.x += std::sin(_target_camera_rotation.y) * elapsed_time * speed;
                        _target_camera_position.z += std::cos(_target_camera_rotation.y) * elapsed_time * speed;
                    }

                    if (input::is_key_down(keycode::d)) {
                        _target_camera_position.x -= std::sin(_target_camera_rotation.y - pi<float>() * 0.5f) * elapsed_time * speed;
                        _target_camera_position.z -= std::cos(_target_camera_rotation.y - pi<float>() * 0.5f) * elapsed_time * speed;
                    } else if (input::is_key_down(keycode::a)) {
                        _target_camera_position.x -= std::sin(_target_camera_rotation.y + pi<float>() * 0.5f) * elapsed_time * speed;
                        _target_camera_position.z -= std::cos(_target_camera_rotation.y + pi<float>() * 0.5f) * elapsed_time * speed;
                    }
                });
            }

            registry.patch<rb::transform>(entity, [this, elapsed_time](rb::transform& transform) {
                transform.position = _smooth(transform.position, _target_camera_position, elapsed_time);
                transform.rotation = _smooth(transform.rotation, _target_camera_rotation, elapsed_time);
            });
        }
    }

private:
    vec3f _smooth(const vec3f& origin, const vec3f& target, float elapsed_time) {
        const auto diff = target - origin;
        if (diff != vec3f::zero()) {
            const auto len = length(diff);
            if (len > 0.0f) {
                const auto dir = diff / len;
                const auto ftr = std::max(len * 40.0f, 0.01f);
                const auto off = std::min(elapsed_time * ftr, len);
                return origin + dir * off;
            }
        }
        return target;
    }

private:
    vec2i _last_mouse_position;
    vec3f _target_camera_position{ 0.0f, 0.5f, 0.0f };
    vec3f _target_camera_rotation{ 0.0f, -pi<float>() * 0.5f, 0.0f };
};

class fps_meter : public rb::system {
public:
    void update(registry& registry, float elapsed_time) override {
        _time += elapsed_time;
        if (_time > 1.0f) {
            window::set_title(format("RabBit FPS: {}", _fps));
            _fps = 0;
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
