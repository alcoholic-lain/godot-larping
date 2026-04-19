#include "win_platform.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

using namespace godot;

void WinPlatform::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_body_entered", "body"), &WinPlatform::_on_body_entered);
    ClassDB::bind_method(D_METHOD("_on_body_exited",  "body"), &WinPlatform::_on_body_exited);
    ClassDB::bind_method(D_METHOD("do_win"),                   &WinPlatform::do_win);
}

void WinPlatform::_ready() {
    CollisionShape2D *phys_shape = memnew(CollisionShape2D);
    Ref<RectangleShape2D> phys_rect = memnew(RectangleShape2D);
    phys_rect->set_size(Vector2(200, 24));
    phys_shape->set_shape(phys_rect);
    add_child(phys_shape);

    ColorRect *visual = memnew(ColorRect);
    visual->set_size(Vector2(200, 24));
    visual->set_position(Vector2(-100, -12));
    visual->set_color(Color(1.0f, 0.85f, 0.0f, 1.0f));
    add_child(visual);

    Area2D *area = memnew(Area2D);
    area->set_position(Vector2(0, -24));
    area->set_collision_mask(1);
    area->set_collision_layer(0);

    CollisionShape2D *area_shape = memnew(CollisionShape2D);
    Ref<RectangleShape2D> area_rect = memnew(RectangleShape2D);
    area_rect->set_size(Vector2(80, 20));
    area_shape->set_shape(area_rect);
    area->add_child(area_shape);

    area->connect("body_entered", Callable(this, "_on_body_entered"));
    area->connect("body_exited",  Callable(this, "_on_body_exited"));
    add_child(area);
}

void WinPlatform::_on_body_entered(Node2D *body) {
    if (won) return;
    if (body->get_name() == StringName("Player")) {
        won = true;
        call_deferred("do_win");
    }
}

void WinPlatform::_on_body_exited(Node2D *body) {
    if (body->get_name() == StringName("Player")) {
        UtilityFunctions::print("exited win zone");
    }
}

void WinPlatform::do_win() {
    get_tree()->change_scene_to_file("res://win_screen.tscn");
}
