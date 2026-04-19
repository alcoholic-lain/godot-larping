#pragma once
#include <godot_cpp/classes/static_body2d.hpp>
#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

class WinPlatform : public StaticBody2D {
    GDCLASS(WinPlatform, StaticBody2D)

protected:
    static void _bind_methods();

public:
    void _ready() override;
    void _on_body_entered(Node2D *body);
    void _on_body_exited(Node2D *body);
private:
    bool won = false;
};

} // namespace godot