#include "spike.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/object.hpp>
#include "player.h"

using namespace godot;

void Spike::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_body_entered", "body"), &Spike::_on_body_entered);
    ClassDB::bind_method(D_METHOD("do_kill"), &Spike::do_kill);

    ClassDB::bind_method(D_METHOD("set_spike_color", "color"), &Spike::set_spike_color);
    ClassDB::bind_method(D_METHOD("get_spike_color"), &Spike::get_spike_color);

    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "spike_color"), "set_spike_color", "get_spike_color");
}

void Spike::_ready() {
    connect("body_entered", Callable(this, "_on_body_entered"));

    // Add collision shape
    CollisionShape2D* collision_shape = memnew(CollisionShape2D);
    RectangleShape2D* shape = memnew(RectangleShape2D);
    shape->set_size(Vector2(24, 24));
    collision_shape->set_shape(shape);
    add_child(collision_shape);

    // Add visual representation (triangle-like spike)
    visual = memnew(Polygon2D);
    PackedVector2Array points;
    points.push_back(Vector2(0, 0));
    points.push_back(Vector2(-16, 32));
    points.push_back(Vector2(16, 32));
    visual->set_polygon(points);
    visual->set_color(spike_color);
    add_child(visual);

    // Enable monitoring
    set_monitoring(true);
    set_collision_mask(1);
}

void Spike::_on_body_entered(Node* body) {
    if (body->get_name() == StringName("Player")) {
        Player* player = Object::cast_to<Player>(body);
        if (player) {
            Color player_color = player->get_color();
            if (player_color == spike_color) {
                UtilityFunctions::print("Player matches spike color, survives!");
                return;
            }
        }
        UtilityFunctions::print("Spike touched!");
        call_deferred("do_kill");
    }
}

void Spike::do_kill() {
    get_tree()->change_scene_to_file("res://lose_screen.tscn");
}