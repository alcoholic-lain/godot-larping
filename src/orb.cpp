#include "orb.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include "player.h"

using namespace godot;

void Orb::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_body_entered", "body"), &Orb::_on_body_entered);
    ClassDB::bind_method(D_METHOD("set_obj_name", "name"), &Orb::set_obj_name);
    ClassDB::bind_method(D_METHOD("get_obj_name"), &Orb::get_obj_name);
    ClassDB::bind_method(D_METHOD("set_color", "color"), &Orb::set_color);
    ClassDB::bind_method(D_METHOD("get_color"), &Orb::get_color);
    ClassDB::bind_method(D_METHOD("set_float_amplitude", "amplitude"), &Orb::set_float_amplitude);
    ClassDB::bind_method(D_METHOD("get_float_amplitude"), &Orb::get_float_amplitude);
    ClassDB::bind_method(D_METHOD("set_float_frequency", "frequency"), &Orb::set_float_frequency);
    ClassDB::bind_method(D_METHOD("get_float_frequency"), &Orb::get_float_frequency);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "obj_name"), "set_obj_name", "get_obj_name");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "float_amplitude"), "set_float_amplitude", "get_float_amplitude");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "float_frequency"), "set_float_frequency", "get_float_frequency");
}

void Orb::_ready() {
    connect("body_entered", Callable(this, "_on_body_entered"));

    // Store original Y position
    original_y = get_position().y;

    // Add visual representation (smaller)
    visual = memnew(ColorRect);
    visual->set_size(Vector2(16, 16));
    visual->set_position(Vector2(-8, -8)); // Center it
    visual->set_color(color);
    add_child(visual);

    // Enable processing for floating
    set_process(true);
}

void Orb::_process(double delta) {
    time_passed += delta;
    float offset_y = sin(time_passed * float_frequency) * float_amplitude;
    Vector2 pos = get_position();
    pos.y = original_y + offset_y;
    set_position(pos);
}

void Orb::_on_body_entered(Node* body) {
    if (body->get_class() == "Player") {
        // Print to console
        UtilityFunctions::print(obj_name + " collected");

        // Change player color
        Player* player = Object::cast_to<Player>(body);
        if (player) {
            player->set_color(color);
        }

        // Update UI label if exists
        Node* parent = get_parent();
        if (parent) {
            Label* label = parent->get_node<Label>(NodePath("CanvasLayer/CollectedLabel"));
            if (label) {
                String current = label->get_text();
                label->set_text(current + obj_name + "\n");
            }
        }

        // Remove the orb
        queue_free();
    }
}