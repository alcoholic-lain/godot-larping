// src/mnms_level_resource.cpp
#include "mnms_level_resource.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void MnmsLevelResource::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_level_name", "level_name"), &MnmsLevelResource::set_level_name);
    ClassDB::bind_method(D_METHOD("get_level_name"), &MnmsLevelResource::get_level_name);
    ClassDB::bind_method(D_METHOD("set_world_size", "world_size"), &MnmsLevelResource::set_world_size);
    ClassDB::bind_method(D_METHOD("get_world_size"), &MnmsLevelResource::get_world_size);
    ClassDB::bind_method(D_METHOD("set_time_limit", "time_limit"), &MnmsLevelResource::set_time_limit);
    ClassDB::bind_method(D_METHOD("get_time_limit"), &MnmsLevelResource::get_time_limit);
    ClassDB::bind_method(D_METHOD("set_recordings_count", "recordings_count"), &MnmsLevelResource::set_recordings_count);
    ClassDB::bind_method(D_METHOD("get_recordings_count"), &MnmsLevelResource::get_recordings_count);
    ClassDB::bind_method(D_METHOD("set_tiles", "tiles"), &MnmsLevelResource::set_tiles);
    ClassDB::bind_method(D_METHOD("get_tiles"), &MnmsLevelResource::get_tiles);
    ClassDB::bind_method(D_METHOD("set_metadata", "metadata"), &MnmsLevelResource::set_metadata);
    ClassDB::bind_method(D_METHOD("get_metadata"), &MnmsLevelResource::get_metadata);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "level_name"), "set_level_name", "get_level_name");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "world_size"), "set_world_size", "get_world_size");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "time_limit"), "set_time_limit", "get_time_limit");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "recordings_count"), "set_recordings_count", "get_recordings_count");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "tiles"), "set_tiles", "get_tiles");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "metadata"), "set_metadata", "get_metadata");
}

MnmsLevelResource::MnmsLevelResource() {
    world_size = Vector2i(800, 600);
    time_limit = -1;
    recordings_count = -1;
}

MnmsLevelResource::~MnmsLevelResource() {}

void MnmsLevelResource::set_level_name(const String &p_level_name) {
    level_name = p_level_name;
}

String MnmsLevelResource::get_level_name() const {
    return level_name;
}

void MnmsLevelResource::set_world_size(const Vector2i &p_world_size) {
    world_size = p_world_size;
}

Vector2i MnmsLevelResource::get_world_size() const {
    return world_size;
}

void MnmsLevelResource::set_time_limit(int32_t p_time_limit) {
    time_limit = p_time_limit;
}

int32_t MnmsLevelResource::get_time_limit() const {
    return time_limit;
}

void MnmsLevelResource::set_recordings_count(int32_t p_recordings_count) {
    recordings_count = p_recordings_count;
}

int32_t MnmsLevelResource::get_recordings_count() const {
    return recordings_count;
}

void MnmsLevelResource::set_tiles(const Array &p_tiles) {
    tiles = p_tiles;
}

Array MnmsLevelResource::get_tiles() const {
    return tiles;
}

void MnmsLevelResource::set_metadata(const Dictionary &p_metadata) {
    metadata = p_metadata;
}

Dictionary MnmsLevelResource::get_metadata() const {
    return metadata;
}
