// src/mnms_pack_resource.cpp
#include "mnms_pack_resource.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void MnmsPackResource::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_pack_id", "pack_id"), &MnmsPackResource::set_pack_id);
    ClassDB::bind_method(D_METHOD("get_pack_id"), &MnmsPackResource::get_pack_id);
    ClassDB::bind_method(D_METHOD("set_name", "name"), &MnmsPackResource::set_name);
    ClassDB::bind_method(D_METHOD("get_name"), &MnmsPackResource::get_name);
    ClassDB::bind_method(D_METHOD("set_description", "description"), &MnmsPackResource::set_description);
    ClassDB::bind_method(D_METHOD("get_description"), &MnmsPackResource::get_description);
    ClassDB::bind_method(D_METHOD("set_theme_id", "theme_id"), &MnmsPackResource::set_theme_id);
    ClassDB::bind_method(D_METHOD("get_theme_id"), &MnmsPackResource::get_theme_id);
    ClassDB::bind_method(D_METHOD("set_music_list", "music_list"), &MnmsPackResource::set_music_list);
    ClassDB::bind_method(D_METHOD("get_music_list"), &MnmsPackResource::get_music_list);
    ClassDB::bind_method(D_METHOD("set_congratulations", "congratulations"), &MnmsPackResource::set_congratulations);
    ClassDB::bind_method(D_METHOD("get_congratulations"), &MnmsPackResource::get_congratulations);
    ClassDB::bind_method(D_METHOD("set_levels", "levels"), &MnmsPackResource::set_levels);
    ClassDB::bind_method(D_METHOD("get_levels"), &MnmsPackResource::get_levels);
    ClassDB::bind_method(D_METHOD("set_metadata", "metadata"), &MnmsPackResource::set_metadata);
    ClassDB::bind_method(D_METHOD("get_metadata"), &MnmsPackResource::get_metadata);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "pack_id"), "set_pack_id", "get_pack_id");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "set_name", "get_name");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "description"), "set_description", "get_description");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "theme_id"), "set_theme_id", "get_theme_id");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "music_list"), "set_music_list", "get_music_list");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "congratulations"), "set_congratulations", "get_congratulations");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "levels"), "set_levels", "get_levels");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "metadata"), "set_metadata", "get_metadata");
}

MnmsPackResource::MnmsPackResource() {}

MnmsPackResource::~MnmsPackResource() {}

void MnmsPackResource::set_pack_id(const String &p_pack_id) {
    pack_id = p_pack_id;
}

String MnmsPackResource::get_pack_id() const {
    return pack_id;
}

void MnmsPackResource::set_name(const String &p_name) {
    name = p_name;
}

String MnmsPackResource::get_name() const {
    return name;
}

void MnmsPackResource::set_description(const String &p_description) {
    description = p_description;
}

String MnmsPackResource::get_description() const {
    return description;
}

void MnmsPackResource::set_theme_id(const String &p_theme_id) {
    theme_id = p_theme_id;
}

String MnmsPackResource::get_theme_id() const {
    return theme_id;
}

void MnmsPackResource::set_music_list(const String &p_music_list) {
    music_list = p_music_list;
}

String MnmsPackResource::get_music_list() const {
    return music_list;
}

void MnmsPackResource::set_congratulations(const String &p_congratulations) {
    congratulations = p_congratulations;
}

String MnmsPackResource::get_congratulations() const {
    return congratulations;
}

void MnmsPackResource::set_levels(const Array &p_levels) {
    levels = p_levels;
}

Array MnmsPackResource::get_levels() const {
    return levels;
}

void MnmsPackResource::set_metadata(const Dictionary &p_metadata) {
    metadata = p_metadata;
}

Dictionary MnmsPackResource::get_metadata() const {
    return metadata;
}
