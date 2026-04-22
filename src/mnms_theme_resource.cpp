// src/mnms_theme_resource.cpp
#include "mnms_theme_resource.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void MnmsThemeResource::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_theme_id", "theme_id"), &MnmsThemeResource::set_theme_id);
    ClassDB::bind_method(D_METHOD("get_theme_id"), &MnmsThemeResource::get_theme_id);
    ClassDB::bind_method(D_METHOD("set_name", "name"), &MnmsThemeResource::set_name);
    ClassDB::bind_method(D_METHOD("get_name"), &MnmsThemeResource::get_name);
    ClassDB::bind_method(D_METHOD("set_block_texture_map", "block_texture_map"), &MnmsThemeResource::set_block_texture_map);
    ClassDB::bind_method(D_METHOD("get_block_texture_map"), &MnmsThemeResource::get_block_texture_map);
    ClassDB::bind_method(D_METHOD("set_character_texture_map", "character_texture_map"), &MnmsThemeResource::set_character_texture_map);
    ClassDB::bind_method(D_METHOD("get_character_texture_map"), &MnmsThemeResource::get_character_texture_map);
    ClassDB::bind_method(D_METHOD("set_character_state_map", "character_state_map"), &MnmsThemeResource::set_character_state_map);
    ClassDB::bind_method(D_METHOD("get_character_state_map"), &MnmsThemeResource::get_character_state_map);
    ClassDB::bind_method(D_METHOD("set_metadata", "metadata"), &MnmsThemeResource::set_metadata);
    ClassDB::bind_method(D_METHOD("get_metadata"), &MnmsThemeResource::get_metadata);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "theme_id"), "set_theme_id", "get_theme_id");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "set_name", "get_name");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "block_texture_map"), "set_block_texture_map", "get_block_texture_map");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "character_texture_map"), "set_character_texture_map", "get_character_texture_map");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "character_state_map"), "set_character_state_map", "get_character_state_map");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "metadata"), "set_metadata", "get_metadata");
}

MnmsThemeResource::MnmsThemeResource() {}

MnmsThemeResource::~MnmsThemeResource() {}

void MnmsThemeResource::set_theme_id(const String &p_theme_id) {
    theme_id = p_theme_id;
}

String MnmsThemeResource::get_theme_id() const {
    return theme_id;
}

void MnmsThemeResource::set_name(const String &p_name) {
    name = p_name;
}

String MnmsThemeResource::get_name() const {
    return name;
}

void MnmsThemeResource::set_block_texture_map(const Dictionary &p_block_texture_map) {
    block_texture_map = p_block_texture_map;
}

Dictionary MnmsThemeResource::get_block_texture_map() const {
    return block_texture_map;
}

void MnmsThemeResource::set_character_texture_map(const Dictionary &p_character_texture_map) {
    character_texture_map = p_character_texture_map;
}

Dictionary MnmsThemeResource::get_character_texture_map() const {
    return character_texture_map;
}

void MnmsThemeResource::set_character_state_map(const Dictionary &p_character_state_map) {
    character_state_map = p_character_state_map;
}

Dictionary MnmsThemeResource::get_character_state_map() const {
    return character_state_map;
}

void MnmsThemeResource::set_metadata(const Dictionary &p_metadata) {
    metadata = p_metadata;
}

Dictionary MnmsThemeResource::get_metadata() const {
    return metadata;
}
