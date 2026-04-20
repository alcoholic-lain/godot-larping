#pragma once
#include <godot_cpp/classes/static_body2d.hpp>
#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/color.hpp>

namespace godot {

class Platform : public StaticBody2D {
    GDCLASS(Platform, StaticBody2D)

private:
    Vector2 plat_size  = Vector2(300, 32);
    Color   plat_color = Color(0.05f, 0.76f, 0.85f, 1.0f);

    void rebuild();

protected:
    static void _bind_methods();

public:
    void _ready() override;

    Vector2 get_plat_size()  const { return plat_size; }
    Color   get_plat_color() const { return plat_color; }
    void    set_plat_size(Vector2 s)  { plat_size  = s;  if (is_inside_tree()) rebuild(); }
    void    set_plat_color(Color c)   { plat_color = c;  if (is_inside_tree()) rebuild(); }
};

} // namespace godot
