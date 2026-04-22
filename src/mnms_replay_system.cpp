// src/mnms_replay_system.cpp
#include "mnms_replay_system.h"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void MnmsReplaySystem::_bind_methods() {
    ClassDB::bind_method(D_METHOD("start_recording"), &MnmsReplaySystem::start_recording);
    ClassDB::bind_method(D_METHOD("stop_recording"), &MnmsReplaySystem::stop_recording);
    ClassDB::bind_method(D_METHOD("set_recording", "recording"), &MnmsReplaySystem::set_recording);
    ClassDB::bind_method(D_METHOD("clear"), &MnmsReplaySystem::clear);
    ClassDB::bind_method(D_METHOD("push_frame", "frame"), &MnmsReplaySystem::push_frame);
    ClassDB::bind_method(D_METHOD("set_recorded_frames", "frames"), &MnmsReplaySystem::set_recorded_frames);
    ClassDB::bind_method(D_METHOD("get_recorded_frames"), &MnmsReplaySystem::get_recorded_frames);
    ClassDB::bind_method(D_METHOD("load_from_file", "path"), &MnmsReplaySystem::load_from_file);
    ClassDB::bind_method(D_METHOD("save_to_file", "path", "metadata"), &MnmsReplaySystem::save_to_file, DEFVAL(Dictionary()));
    ClassDB::bind_method(D_METHOD("is_recording"), &MnmsReplaySystem::is_recording);
    ClassDB::bind_method(D_METHOD("get_frame_count"), &MnmsReplaySystem::get_frame_count);
}

MnmsReplaySystem::MnmsReplaySystem() {
    recording = false;
}

MnmsReplaySystem::~MnmsReplaySystem() {}

void MnmsReplaySystem::start_recording() {
    recording = true;
    recorded_frames.clear();
}

void MnmsReplaySystem::stop_recording() {
    recording = false;
}

void MnmsReplaySystem::set_recording(bool p_recording) {
    recording = p_recording;
}

void MnmsReplaySystem::clear() {
    recorded_frames.clear();
}

void MnmsReplaySystem::push_frame(const Dictionary &p_frame) {
    if (recording) {
        recorded_frames.push_back(p_frame);
    }
}

void MnmsReplaySystem::set_recorded_frames(const Array &p_frames) {
    recorded_frames = p_frames;
}

Array MnmsReplaySystem::get_recorded_frames() const {
    return recorded_frames;
}

Dictionary MnmsReplaySystem::load_from_file(const String &p_path) {
    Dictionary result;
    Ref<ConfigFile> config;
    config.instantiate();
    if (config->load(p_path) != OK) {
        result["ok"] = false;
        return result;
    }

    const String format = config->get_value("replay", "format", "");
    if (format != "mnms_replay_v1") {
        result["ok"] = false;
        result["error"] = "unsupported_format";
        return result;
    }

    recording = false;
    recorded_frames = config->get_value("replay", "frames", Array());
    result["ok"] = true;
    result["frame_count"] = recorded_frames.size();

    Array section_keys = config->get_section_keys("meta");
    for (int i = 0; i < section_keys.size(); i++) {
        const String key = section_keys[i];
        result[key] = config->get_value("meta", key);
    }
    return result;
}

bool MnmsReplaySystem::save_to_file(const String &p_path, const Dictionary &p_metadata) const {
    Ref<ConfigFile> config;
    config.instantiate();
    config->set_value("replay", "format", "mnms_replay_v1");
    config->set_value("replay", "recording", recording);
    config->set_value("replay", "frame_count", recorded_frames.size());
    config->set_value("replay", "frames", recorded_frames);

    Array metadata_keys = p_metadata.keys();
    for (int i = 0; i < metadata_keys.size(); i++) {
        const Variant key_variant = metadata_keys[i];
        const String key = String(key_variant);
        config->set_value("meta", key, p_metadata[key_variant]);
    }

    return config->save(p_path) == OK;
}

bool MnmsReplaySystem::is_recording() const {
    return recording;
}

int32_t MnmsReplaySystem::get_frame_count() const {
    return recorded_frames.size();
}
