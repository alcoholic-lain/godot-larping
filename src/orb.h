#pragma once
#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/label.hpp>

namespace godot {

class Orb : public Area2D {
    GDCLASS(Orb, Area2D)

private:
    String obj_name = "Orb";
    Color color = Color(1.0f, 1.0f, 1.0f, 1.0f);
    ColorRect* visual = nullptr;
    float original_y = 0.0f;
    float time_passed = 0.0f;
    float float_amplitude = 5.0f;
    float float_frequency = 2.0f;

protected:
    static void _bind_methods();

public:
    void _ready() override;
    void _process(double delta) override;
    void _on_body_entered(Node* body);

    void set_obj_name(const String& name) { obj_name = name; }
    String get_obj_name() const { return obj_name; }

    void set_color(const Color& c) { color = c; if (visual) visual->set_color(c); }
    Color get_color() const { return color; }

    void set_float_amplitude(float a) { float_amplitude = a; }
    float get_float_amplitude() const { return float_amplitude; }

    void set_float_frequency(float f) { float_frequency = f; }
    float get_float_frequency() const { return float_frequency; }
};

} // namespace godot