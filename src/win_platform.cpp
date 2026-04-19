#include "win_platform.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void WinPlatform::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_body_entered", "body"), &WinPlatform::_on_body_entered);
    ClassDB::bind_method(D_METHOD("_on_body_exited",  "body"), &WinPlatform::_on_body_exited);
}

void WinPlatform::_ready() {
    // Physical collision shape — player can stand on this
    CollisionShape2D *phys_shape = memnew(CollisionShape2D);
    Ref<RectangleShape2D> phys_rect = memnew(RectangleShape2D);
    phys_rect->set_size(Vector2(200, 24));
    phys_shape->set_shape(phys_rect);
    add_child(phys_shape);

    // Visual
    ColorRect *visual = memnew(ColorRect);
    visual->set_size(Vector2(200, 24));
    visual->set_position(Vector2(-100, -12));
    visual->set_color(Color(1.0f, 0.85f, 0.0f, 1.0f));
    add_child(visual);

    // Area2D on top of the platform — smaller and centered
    // only covers the center 80px so player must be centered
    Area2D *area = memnew(Area2D);
    area->set_position(Vector2(0, -24)); // sits just above the platform surface

    // Only detect layer 1 (player should be on layer 1)
    area->set_collision_mask(1);
    area->set_collision_layer(0); // area itself is on no layer

    CollisionShape2D *area_shape = memnew(CollisionShape2D);
    Ref<RectangleShape2D> area_rect = memnew(RectangleShape2D);
    area_rect->set_size(Vector2(80, 20)); // narrow = must be centered
    area_shape->set_shape(area_rect);
    area->add_child(area_shape);

    area->connect("body_entered", Callable(this, "_on_body_entered"));
    area->connect("body_exited",  Callable(this, "_on_body_exited"));
    add_child(area);
}

void WinPlatform::_on_body_entered(Node2D *body) {
    if (won) return;
    
    // Print what's actually entering so we can see
    UtilityFunctions::print("body entered: ", body->get_class(), " name: ", body->get_name());
    
    if (body->get_name() == StringName("Player")) {
        won = true;
        UtilityFunctions::print("YOU WIN!");
    }
}

void WinPlatform::_on_body_exited(Node2D *body) {
    if (body->get_name() == StringName("Player")) {
        UtilityFunctions::print("exited win zone ");
    }
}