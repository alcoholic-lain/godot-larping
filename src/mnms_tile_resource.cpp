// src/mnms_tile_resource.cpp
#include "mnms_tile_resource.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void MnmsTileResource::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_block_type", "block_type"), &MnmsTileResource::set_block_type);
    ClassDB::bind_method(D_METHOD("get_block_type"), &MnmsTileResource::get_block_type);
    ClassDB::bind_method(D_METHOD("set_position", "position"), &MnmsTileResource::set_position);
    ClassDB::bind_method(D_METHOD("get_position"), &MnmsTileResource::get_position);
    ClassDB::bind_method(D_METHOD("set_size", "size"), &MnmsTileResource::set_size);
    ClassDB::bind_method(D_METHOD("get_size"), &MnmsTileResource::get_size);
    ClassDB::bind_method(D_METHOD("set_properties", "properties"), &MnmsTileResource::set_properties);
    ClassDB::bind_method(D_METHOD("get_properties"), &MnmsTileResource::get_properties);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "block_type"), "set_block_type", "get_block_type");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "position"), "set_position", "get_position");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "size"), "set_size", "get_size");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "properties"), "set_properties", "get_properties");
}

MnmsTileResource::MnmsTileResource() {
    size = Vector2i(50, 50);
}

MnmsTileResource::~MnmsTileResource() {}

void MnmsTileResource::set_block_type(const String &p_block_type) {
    block_type = p_block_type;
}

String MnmsTileResource::get_block_type() const {
    return block_type;
}

void MnmsTileResource::set_position(const Vector2i &p_position) {
    position = p_position;
}

Vector2i MnmsTileResource::get_position() const {
    return position;
}

void MnmsTileResource::set_size(const Vector2i &p_size) {
    size = p_size;
}

Vector2i MnmsTileResource::get_size() const {
    return size;
}

void MnmsTileResource::set_properties(const Dictionary &p_properties) {
    properties = p_properties;
}

Dictionary MnmsTileResource::get_properties() const {
    return properties;
}
