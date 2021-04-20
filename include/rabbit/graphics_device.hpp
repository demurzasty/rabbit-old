#pragma once 

#include "config.hpp"
#include "di.hpp"
#include "vec4.hpp"
#include "buffer.hpp"
#include "mesh.hpp"
#include "material.hpp"
#include "resource_heap.hpp"
#include "texture.hpp"

#include <memory>

namespace rb {
    class graphics_device {
    public:
        /**
		 * @brief Install window implementation to dependency container.
		 */
		static void install(installer<graphics_device>& installer);
        
		/**
		 * @brief Default virtual destructor.
		 */
        virtual ~graphics_device() = default;

        RB_NODISCARD virtual std::shared_ptr<buffer> make_buffer(const buffer_desc& desc) = 0;

        RB_NODISCARD virtual std::shared_ptr<mesh> make_mesh(const mesh_desc& desc) = 0;

        RB_NODISCARD virtual std::shared_ptr<material> make_material(const material_desc& desc) = 0;

        RB_NODISCARD virtual std::shared_ptr<resource_heap> make_resource_heap(const resource_heap_desc& desc) = 0;

        RB_NODISCARD virtual std::shared_ptr<texture> make_texture(const texture_desc& desc) = 0;

        virtual void begin() = 0;

        virtual void end() = 0;

        virtual void begin_render_pass() = 0;

        virtual void end_render_pass() = 0;

        virtual void update_buffer(const std::shared_ptr<buffer>& buffer, const void* data, std::size_t offset, std::size_t size) = 0;

        virtual void set_viewport(const vec4i& viewport) = 0;

        virtual void draw(const std::shared_ptr<mesh>& mesh, const std::shared_ptr<material>& material, const std::shared_ptr<resource_heap>& resource_heap) = 0;

        virtual void present() = 0;
    };
}
