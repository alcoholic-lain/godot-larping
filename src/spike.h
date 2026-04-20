#pragma once
#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>

namespace godot {

class Spike : public Area2D {
    GDCLASS(Spike, Area2D)

private:
    Polygon2D* visual = nullptr;
    Color spike_color = Color(1.0f, 0.0f, 0.0f, 1.0f);
    
    // Geometry properties
    Vector2 spike_size = Vector2(32, 32);     // Visual width/height
    Vector2 hitbox_size = Vector2(20, 24);    // Collision width/height
    Vector2 hitbox_offset = Vector2(0, 20);   // Collision position relative to tip

protected:
    static void _bind_methods();

public:
    void _ready() override;
    void rebuild();
    void _on_body_entered(Node* body);
    void do_kill();

    // Setters & Getters
    void set_spike_color(Color c) { spike_color = c; if (visual) visual->set_color(c); }
    Color get_spike_color() const { return spike_color; }

    void set_spike_size(Vector2 s) { spike_size = s; rebuild(); }
    Vector2 get_spike_size() const { return spike_size; }

    void set_hitbox_size(Vector2 s) { hitbox_size = s; rebuild(); }
    Vector2 get_hitbox_size() const { return hitbox_size; }

    void set_hitbox_offset(Vector2 o) { hitbox_offset = o; rebuild(); }
    Vector2 get_hitbox_offset() const { return hitbox_offset; }
};

} // namespace godot