#pragma once
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/static_body2d.hpp>
#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot {

class Frame : public Node2D {
    GDCLASS(Frame, Node2D)

private:
    float level_width    = 1152.0f;
    float level_height   = 648.0f;
    float wall_thickness = 24.0f;

    void create_bounds();
    void create_wall(Vector2 position, Vector2 size, const String &name);

protected:
    static void _bind_methods();

public:
    void _ready() override;

    float get_level_width()  const { return level_width; }
    float get_level_height() const { return level_height; }
    void  set_level_width(float w)  { level_width = w; }
    void  set_level_height(float h) { level_height = h; }
};

} // namespace godot