#include "collectable.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include "player.h"

using namespace godot;

void Collectable::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_body_entered", "body"), &Collectable::_on_body_entered);
    ClassDB::bind_method(D_METHOD("set_obj_name", "name"), &Collectable::set_obj_name);
    ClassDB::bind_method(D_METHOD("get_obj_name"), &Collectable::get_obj_name);
    ClassDB::bind_method(D_METHOD("set_color", "color"), &Collectable::set_color);
    ClassDB::bind_method(D_METHOD("get_color"), &Collectable::get_color);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "obj_name"), "set_obj_name", "get_obj_name");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");
}

void Collectable::_ready() {
    connect("body_entered", Callable(this, "_on_body_entered"));

    // Add visual representation
    visual = memnew(ColorRect);
    visual->set_size(Vector2(32, 32));
    visual->set_position(Vector2(-16, -16)); // Center it
    visual->set_color(color);
    add_child(visual);
}

void Collectable::_on_body_entered(Node* body) {
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

        // Remove the collectable
        queue_free();
    }
}