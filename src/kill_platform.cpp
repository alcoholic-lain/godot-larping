#include "kill_platform.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/engine.hpp>

using namespace godot;

void KillPlatform::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_body_entered", "body"), &KillPlatform::_on_body_entered);
    ClassDB::bind_method(D_METHOD("do_lose"),                   &KillPlatform::do_lose);
}

void KillPlatform::_ready() {
    CollisionShape2D *phys_shape = memnew(CollisionShape2D);
    Ref<RectangleShape2D> phys_rect = memnew(RectangleShape2D);
    phys_rect->set_size(Vector2(32, 32));
    phys_shape->set_shape(phys_rect);
    add_child(phys_shape);

    ColorRect *visual = memnew(ColorRect);
    visual->set_size(Vector2(32, 32));
    visual->set_position(Vector2(-16, -16));
    visual->set_color(Color(0.8f, 0.0f, 0.0f, 1.0f)); // Red
    if (!Engine::get_singleton()->is_editor_hint()) {
        visual->set_visible(false);
    }
    add_child(visual);

    Area2D *area = memnew(Area2D);
    area->set_position(Vector2(0, 0));
    area->set_collision_mask(1);
    area->set_collision_layer(0);

    CollisionShape2D *area_shape = memnew(CollisionShape2D);
    Ref<RectangleShape2D> area_rect = memnew(RectangleShape2D);
    area_rect->set_size(Vector2(32, 32));
    area_shape->set_shape(area_rect);
    area->add_child(area_shape);

    area->connect("body_entered", Callable(this, "_on_body_entered"));
    add_child(area);
}

void KillPlatform::_on_body_entered(Node2D *body) {
    if (body->get_name() == StringName("Player")) {
        call_deferred("do_lose");
    }
}

void KillPlatform::do_lose() {
    get_tree()->change_scene_to_file("res://lose_screen.tscn");
}