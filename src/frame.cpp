#include "frame.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/static_body2d.hpp>
#include <godot_cpp/classes/engine.hpp>

using namespace godot;

void Frame::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_level_width"),       &Frame::get_level_width);
    ClassDB::bind_method(D_METHOD("get_level_height"),      &Frame::get_level_height);
    ClassDB::bind_method(D_METHOD("set_level_width",  "w"), &Frame::set_level_width);
    ClassDB::bind_method(D_METHOD("set_level_height", "h"), &Frame::set_level_height);
    ClassDB::bind_method(D_METHOD("rebuild"),               &Frame::rebuild);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "level_width"),  "set_level_width",  "get_level_width");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "level_height"), "set_level_height", "get_level_height");
}

void Frame::set_level_width(float w) {
    level_width = w;
    rebuild();
}

void Frame::set_level_height(float h) {
    level_height = h;
    rebuild();
}

void Frame::_ready() {
    rebuild();
}

void Frame::rebuild() {
    // 1. Precise Cleanup
    for (int i = get_child_count() - 1; i >= 0; i--) {
        Node *child = get_child(i);
        String name = child->get_name();

        // Check for exact name matches only
        if (name == "FloorWall" || name == "CeilingWall" || 
            name == "LeftWall"  || name == "RightWall") {
            
            remove_child(child);
            memdelete(child); 
        }
    }

    float w = level_width;
    float h = level_height;
    float t = wall_thickness;

    // 2. Re-create boundaries with exact names
    // Note: create_wall must assign these names for the next rebuild to find them
    create_wall(Vector2(w / 2, h + t / 2),  Vector2(w + t * 2, t), "FloorWall");
    create_wall(Vector2(w / 2, -t / 2),      Vector2(w + t * 2, t), "CeilingWall");
    create_wall(Vector2(-t / 2, h / 2),      Vector2(t, h + t * 2), "LeftWall");
    create_wall(Vector2(w + t / 2, h / 2),   Vector2(t, h + t * 2), "RightWall");
}

void Frame::create_wall(Vector2 position, Vector2 size, const String &name) {
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