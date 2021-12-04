#pragma once 

#include "../core/entity.hpp"
#include "../platform/window.hpp"
#include "../math/mat4.hpp"
#include "instance.hpp"
#include "mesh.hpp"
#include "material.hpp"
#include "texture.hpp"

#include <cstdint>
#include <memory>

namespace rb {
    class graphics_impl {
    public:
        virtual ~graphics_impl() = default;

        virtual void set_camera_projection_matrix(const mat4f& projection_matrix) = 0;

        virtual void set_camera_view_matrix(const mat4f& view_matrix) = 0;

        virtual void set_camera_world_matrix(const mat4f& world_matrix) = 0;

        virtual instance make_instance();

        virtual void destroy_instance(instance instance);

        virtual void set_instance_mesh(instance instance, const std::shared_ptr<mesh>& mesh) = 0;

        virtual void set_instance_material(instance instance, const std::shared_ptr<material>& material) = 0;

        virtual void set_instance_world_matrix(instance instance, const mat4f& world_matrix) = 0;

        virtual void draw() = 0;

        virtual void swap_buffers() = 0;

    protected:
        basic_registry<instance>& instance_registry();

    private:
        basic_registry<instance> _instance_registry;
    };

    class graphics {
    public:
        static void setup();

        static void release();

        static void set_camera_projection_matrix(const mat4f& projection_matrix);

        static void set_camera_view_matrix(const mat4f& view_matrix);

        static void set_camera_world_matrix(const mat4f& world_matrix);

        static instance make_instance();

        static void destroy_instance(instance instance);

        static void set_instance_mesh(instance instance, const std::shared_ptr<mesh>& mesh);

        static void set_instance_material(instance instance, const std::shared_ptr<material>& material);

        static void set_instance_world_matrix(instance instance, const mat4f& world_matrix);

        static void draw();

        static void swap_buffers();

    private:
        static std::shared_ptr<graphics_impl> _impl;
    };
}
