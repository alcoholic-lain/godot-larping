#include "level.h"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void Level::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_level_width"),  &Level::get_level_width);
    ClassDB::bind_method(D_METHOD("get_level_height"), &Level::get_level_height);
    ClassDB::bind_method(D_METHOD("set_level_width",  "w"), &Level::set_level_width);
    ClassDB::bind_method(D_METHOD("set_level_height", "h"), &Level::set_level_height);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "level_width"),  "set_level_width",  "get_level_width");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "level_height"), "set_level_height", "get_level_height");
}

void Level::_ready() {
    create_bounds();
}

void Level::create_bounds() {
    float w = level_width;
    float h = level_height;
    float t = wall_thickness;

    // Floor
    create_wall(Vector2(w / 2, h + t / 2),   Vector2(w + t * 2, t), "FloorWall");
    // Ceiling
    create_wall(Vector2(w / 2, -t / 2),       Vector2(w + t * 2, t), "CeilingWall");
    // Left wall
    create_wall(Vector2(-t / 2, h / 2),       Vector2(t, h + t * 2), "LeftWall");
    // Right wall
    create_wall(Vector2(w + t / 2, h / 2),    Vector2(t, h + t * 2), "RightWall");
}

void Level::create_wall(Vector2 position, Vector2 size, const String &name) {
    StaticBody2D *body = memnew(StaticBody2D);
    body->set_name(name);
    body->set_position(position);

    CollisionShape2D *shape_node = memnew(CollisionShape2D);
    Ref<RectangleShape2D> rect = memnew(RectangleShape2D);
    rect->set_size(size);
    shape_node->set_shape(rect);

    body->add_child(shape_node);
    add_child(body);
}