#pragma once
#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/color_rect.hpp>

namespace godot {

class Collectable : public Area2D {
    GDCLASS(Collectable, Area2D)

private:
    String obj_name = "Item";
    Color color = Color(1.0f, 1.0f, 1.0f, 1.0f);
    ColorRect* visual = nullptr;

protected:
    static void _bind_methods();

public:
    void _ready() override;
    void _on_body_entered(Node* body);

    void set_obj_name(const String& name) { obj_name = name; }
    String get_obj_name() const { return obj_name; }

    void set_color(const Color& c) { color = c; if (visual) visual->set_color(c); }
    Color get_color() const { return color; }
};

} // namespace godot