#include "spike.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/object.hpp>
#include "player.h"

using namespace godot;

void Spike::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_body_entered", "body"), &Spike::_on_body_entered);
    ClassDB::bind_method(D_METHOD("do_kill"), &Spike::do_kill);
    ClassDB::bind_method(D_METHOD("rebuild"), &Spike::rebuild);

    // Color Property
    ClassDB::bind_method(D_METHOD("set_spike_color", "color"), &Spike::set_spike_color);
    ClassDB::bind_method(D_METHOD("get_spike_color"), &Spike::get_spike_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "spike_color"), "set_spike_color", "get_spike_color");

    // Visual Size Property
    ClassDB::bind_method(D_METHOD("set_spike_size", "size"), &Spike::set_spike_size);
    ClassDB::bind_method(D_METHOD("get_spike_size"), &Spike::get_spike_size);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "spike_size"), "set_spike_size", "get_spike_size");

    // Hitbox Size Property
    ClassDB::bind_method(D_METHOD("set_hitbox_size", "size"), &Spike::set_hitbox_size);
    ClassDB::bind_method(D_METHOD("get_hitbox_size"), &Spike::get_hitbox_size);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "hitbox_size"), "set_hitbox_size", "get_hitbox_size");

    // Hitbox Offset Property
    ClassDB::bind_method(D_METHOD("set_hitbox_offset", "offset"), &Spike::set_hitbox_offset);
    ClassDB::bind_method(D_METHOD("get_hitbox_offset"), &Spike::get_hitbox_offset);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "hitbox_offset"), "set_hitbox_offset", "get_hitbox_offset");
}

void Spike::_ready() {
    if (!is_connected("body_entered", Callable(this, "_on_body_entered"))) {
        connect("body_entered", Callable(this, "_on_body_entered"));
    }
    set_monitoring(true);
    set_collision_mask(1); // Layer 1 (Player)
    rebuild();
}

void Spike::rebuild() {
    // 1. Remove old children
    for (int i = get_child_count() - 1; i >= 0; i--) {
        Node* child = get_child(i);
        if (Object::cast_to<CollisionShape2D>(child) || Object::cast_to<Polygon2D>(child)) {
            child->queue_free();
        }
    }

    // 2. Create the Hitbox (Collision)
    CollisionShape2D* col_shape = memnew(CollisionShape2D);
    Ref<RectangleShape2D> rect = memnew(RectangleShape2D);
    
    rect->set_size(hitbox_size);
    col_shape->set_shape(rect);
    col_shape->set_position(hitbox_offset); // Apply custom offset
    add_child(col_shape);

    // 3. Create the Visual (Triangle)
    visual = memnew(Polygon2D);
    float w = spike_size.x;
    float h = spike_size.y;
    
    PackedVector2Array points;
    points.push_back(Vector2(0, 0));       // Tip at origin
    points.push_back(Vector2(-w / 2, h));  // Bottom Left
    points.push_back(Vector2(w / 2, h));   // Bottom Right
    
    visual->set_polygon(points);
    visual->set_color(spike_color);
    add_child(visual);
}

void Spike::_on_body_entered(Node* body) {
    if (body->get_name() == StringName("Player")) {
        Player* player = Object::cast_to<Player>(body);
        if (player && player->get_color() == spike_color) {
            return; // Survival logic
        }
        call_deferred("do_kill");
    }
}

void Spike::do_kill() {
    get_tree()->change_scene_to_file("res://lose_screen.tscn");
}