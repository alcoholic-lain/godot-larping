#pragma once
#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/classes/polygon2d.hpp>

namespace godot {

class Spike : public Area2D {
    GDCLASS(Spike, Area2D)

private:
    Polygon2D* visual = nullptr;

    Color spike_color = Color(1.0f, 0.0f, 0.0f, 1.0f); // default red

protected:
    static void _bind_methods();

public:
    void _ready() override;
    void _on_body_entered(Node* body);
    void do_kill();

    void set_spike_color(Color c) { spike_color = c; if (visual) visual->set_color(c); }
    Color get_spike_color() const { return spike_color; }
};

} // namespace godot