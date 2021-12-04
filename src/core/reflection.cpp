#include <rabbit/rabbit.hpp>

using namespace rb;

std::unordered_map<std::type_index, type> reflection::_types;

void reflection::deserialize(std::type_index type_index, void* data, const json& json) {
    auto& type = _types.at(type_index);

    for (const auto& [field_name, field] : type.fields) {

    }
}

void app::_reflect() {
    reflection::reflect<camera_type>()
        .name("camera_type")
        .value<camera_type::perspective>("perspective")
        .value<camera_type::orthogonal>("orthogonal");

    reflection::reflect<camera>()
        .name("camera")
        .field<&camera::type>("type")
        .field<&camera::size>("size")
        .field<&camera::field_of_view>("field_of_view")
        .field<&camera::z_near>("z_near")
        .field<&camera::z_far>("z_far");

    reflection::reflect<local_transform>()
        .name("local_transform")
        .field<&local_transform::position>("position")
        .field<&local_transform::rotation>("rotation")
        .field<&local_transform::scale>("scale");

    reflection::reflect<identity>()
        .name("identity")
        .field<&identity::name>("name");

    reflection::reflect<geometry>()
        .name("geometry")
        .field<&geometry::mesh>("mesh")
        .field<&geometry::material>("material");

    reflection::reflect<directional_light>()
        .name("directional_light")
        .field<&directional_light::color>("color")
        .field<&directional_light::intensity>("intesity");

    reflection::reflect<point_light>()
        .name("point_light")
        .field<&point_light::color>("color")
        .field<&point_light::intensity>("intesity")
        .field<&point_light::radius>("radius");

    reflection::reflect<spot_light>()
        .name("spot_light")
        .field<&spot_light::color>("color")
        .field<&spot_light::intensity>("intesity")
        .field<&spot_light::angle>("angle")
        .field<&spot_light::range>("range");
}
