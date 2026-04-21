// src/mnms_player_shadow_system.cpp
#include "mnms_player_shadow_system.h"

#include <godot_cpp/classes/camera2d.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void MnmsPlayerShadowSystem::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_player_path", "player_path"), &MnmsPlayerShadowSystem::set_player_path);
    ClassDB::bind_method(D_METHOD("get_player_path"), &MnmsPlayerShadowSystem::get_player_path);
    ClassDB::bind_method(D_METHOD("set_shadow_path", "shadow_path"), &MnmsPlayerShadowSystem::set_shadow_path);
    ClassDB::bind_method(D_METHOD("get_shadow_path"), &MnmsPlayerShadowSystem::get_shadow_path);
    ClassDB::bind_method(D_METHOD("set_replay_system_path", "replay_system_path"), &MnmsPlayerShadowSystem::set_replay_system_path);
    ClassDB::bind_method(D_METHOD("get_replay_system_path"), &MnmsPlayerShadowSystem::get_replay_system_path);
    ClassDB::bind_method(D_METHOD("bind_runtime", "player_path", "shadow_path", "replay_system_path"), &MnmsPlayerShadowSystem::bind_runtime);
    ClassDB::bind_method(D_METHOD("reset_runtime_state"), &MnmsPlayerShadowSystem::reset_runtime_state);
    ClassDB::bind_method(D_METHOD("request_record_toggle"), &MnmsPlayerShadowSystem::request_record_toggle);
    ClassDB::bind_method(D_METHOD("request_record_cancel"), &MnmsPlayerShadowSystem::request_record_cancel);
    ClassDB::bind_method(D_METHOD("capture_checkpoint_state"), &MnmsPlayerShadowSystem::capture_checkpoint_state);
    ClassDB::bind_method(D_METHOD("restore_checkpoint_state"), &MnmsPlayerShadowSystem::restore_checkpoint_state);
    ClassDB::bind_method(D_METHOD("begin_recording"), &MnmsPlayerShadowSystem::begin_recording);
    ClassDB::bind_method(D_METHOD("cancel_recording"), &MnmsPlayerShadowSystem::cancel_recording);
    ClassDB::bind_method(D_METHOD("end_recording_and_replay"), &MnmsPlayerShadowSystem::end_recording_and_replay);
    ClassDB::bind_method(D_METHOD("restart_replay"), &MnmsPlayerShadowSystem::restart_replay);
    ClassDB::bind_method(D_METHOD("stop_replay_playback"), &MnmsPlayerShadowSystem::stop_replay_playback);
    ClassDB::bind_method(D_METHOD("stop_replay"), &MnmsPlayerShadowSystem::stop_replay);
    ClassDB::bind_method(D_METHOD("capture_frame"), &MnmsPlayerShadowSystem::capture_frame);
    ClassDB::bind_method(D_METHOD("toggle_shadow_view"), &MnmsPlayerShadowSystem::toggle_shadow_view);
    ClassDB::bind_method(D_METHOD("is_shadow_view"), &MnmsPlayerShadowSystem::is_shadow_view);
    ClassDB::bind_method(D_METHOD("is_recording"), &MnmsPlayerShadowSystem::is_recording);
    ClassDB::bind_method(D_METHOD("is_replaying"), &MnmsPlayerShadowSystem::is_replaying);
    ClassDB::bind_method(D_METHOD("has_replay"), &MnmsPlayerShadowSystem::has_replay);
    ClassDB::bind_method(D_METHOD("consume_recording_started"), &MnmsPlayerShadowSystem::consume_recording_started);
    ClassDB::bind_method(D_METHOD("consume_recording_cancelled"), &MnmsPlayerShadowSystem::consume_recording_cancelled);
    ClassDB::bind_method(D_METHOD("consume_replay_started"), &MnmsPlayerShadowSystem::consume_replay_started);

    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "player_path"), "set_player_path", "get_player_path");
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "shadow_path"), "set_shadow_path", "get_shadow_path");
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "replay_system_path"), "set_replay_system_path", "get_replay_system_path");
}

MnmsPlayerShadowSystem::MnmsPlayerShadowSystem() {
    shadow_view = false;
    recording_active = false;
    record_toggle_requested = false;
    record_cancel_requested = false;
    recording_started_this_tick = false;
    recording_cancelled_this_tick = false;
    replay_started_this_tick = false;
    checkpoint_state.clear();
    set_process_priority(-100);
    set_physics_process_priority(-100);
    set_physics_process(true);
}

MnmsPlayerShadowSystem::~MnmsPlayerShadowSystem() {}

void MnmsPlayerShadowSystem::set_player_path(const NodePath &p_player_path) {
    player_path = p_player_path;
}

NodePath MnmsPlayerShadowSystem::get_player_path() const {
    return player_path;
}

void MnmsPlayerShadowSystem::set_shadow_path(const NodePath &p_shadow_path) {
    shadow_path = p_shadow_path;
}

NodePath MnmsPlayerShadowSystem::get_shadow_path() const {
    return shadow_path;
}

void MnmsPlayerShadowSystem::set_replay_system_path(const NodePath &p_replay_system_path) {
    replay_system_path = p_replay_system_path;
}

NodePath MnmsPlayerShadowSystem::get_replay_system_path() const {
    return replay_system_path;
}

void MnmsPlayerShadowSystem::bind_runtime(const NodePath &p_player_path, const NodePath &p_shadow_path, const NodePath &p_replay_system_path) {
    player_path = p_player_path;
    shadow_path = p_shadow_path;
    replay_system_path = p_replay_system_path;
    reset_runtime_state();
}

void MnmsPlayerShadowSystem::reset_runtime_state() {
    recording_active = false;
    shadow_view = false;
    record_toggle_requested = false;
    record_cancel_requested = false;
    recording_started_this_tick = false;
    recording_cancelled_this_tick = false;
    replay_started_this_tick = false;

    _clear_input_frame_meta(_get_player());
    _clear_input_frame_meta(_get_shadow());

    MnmsPlayer *player = _get_player();
    if (player != nullptr) {
        player->clear_recording_trail();
        player->set_recording_trail_enabled(false);
    }

    MnmsReplaySystem *replay_system = _get_replay_system();
    if (replay_system != nullptr) {
        replay_system->stop_recording();
        replay_system->clear();
    }

    MnmsShadow *shadow = _get_shadow();
    if (shadow != nullptr) {
        shadow->stop_replay();
        shadow->reset_to_spawn();
    }

    _apply_camera_state();
}

void MnmsPlayerShadowSystem::request_record_toggle() {
    record_toggle_requested = true;
}

void MnmsPlayerShadowSystem::request_record_cancel() {
    record_cancel_requested = true;
}

void MnmsPlayerShadowSystem::capture_checkpoint_state() {
    MnmsReplaySystem *replay_system = _get_replay_system();
    MnmsPlayer *player = _get_player();
    MnmsShadow *shadow = _get_shadow();
    if (replay_system == nullptr || player == nullptr || shadow == nullptr) {
        checkpoint_state.clear();
        return;
    }

    checkpoint_state.clear();
    checkpoint_state["shadow_view"] = shadow_view;
    checkpoint_state["recording_active"] = recording_active;
    checkpoint_state["recorded_frames"] = replay_system->get_recorded_frames();
    checkpoint_state["player_trail_points"] = player->get_recording_trail_points();
    checkpoint_state["player_velocity"] = player->get_velocity();
    checkpoint_state["player_last_teleporter_id"] = player->has_meta("last_teleporter_id") ? Variant(String(player->get_meta("last_teleporter_id"))) : Variant(String());
    checkpoint_state["shadow_replaying"] = shadow->is_replaying();
    checkpoint_state["shadow_replay_frames"] = shadow->get_replay_frames();
    checkpoint_state["shadow_replay_frame_index"] = shadow->get_replay_frame_index();
    checkpoint_state["shadow_position"] = shadow->get_position();
    checkpoint_state["shadow_velocity"] = shadow->get_velocity();
    checkpoint_state["shadow_last_teleporter_id"] = shadow->has_meta("last_teleporter_id") ? Variant(String(shadow->get_meta("last_teleporter_id"))) : Variant(String());
}

void MnmsPlayerShadowSystem::restore_checkpoint_state() {
    if (checkpoint_state.is_empty()) {
        reset_runtime_state();
        return;
    }

    MnmsReplaySystem *replay_system = _get_replay_system();
    MnmsPlayer *player = _get_player();
    MnmsShadow *shadow = _get_shadow();
    if (replay_system == nullptr || player == nullptr || shadow == nullptr) {
        return;
    }

    const Array recorded_frames = checkpoint_state.has("recorded_frames") ? Array(checkpoint_state["recorded_frames"]) : Array();
    const Array player_trail_points = checkpoint_state.has("player_trail_points") ? Array(checkpoint_state["player_trail_points"]) : Array();
    const bool restore_recording = checkpoint_state.has("recording_active") && (bool)checkpoint_state["recording_active"];
    const Vector2 player_velocity = checkpoint_state.has("player_velocity") ? Vector2(checkpoint_state["player_velocity"]) : Vector2();
    const String player_last_teleporter_id = checkpoint_state.has("player_last_teleporter_id") ? String(checkpoint_state["player_last_teleporter_id"]) : String();
    const bool restore_replaying = checkpoint_state.has("shadow_replaying") && (bool)checkpoint_state["shadow_replaying"];
    const Array shadow_replay_frames = checkpoint_state.has("shadow_replay_frames") ? Array(checkpoint_state["shadow_replay_frames"]) : Array();
    const int32_t replay_frame_index = checkpoint_state.has("shadow_replay_frame_index") ? (int32_t)(int)checkpoint_state["shadow_replay_frame_index"] : 0;
    const Vector2 shadow_position = checkpoint_state.has("shadow_position") ? Vector2(checkpoint_state["shadow_position"]) : shadow->get_position();
    const Vector2 shadow_velocity = checkpoint_state.has("shadow_velocity") ? Vector2(checkpoint_state["shadow_velocity"]) : Vector2();
    const String shadow_last_teleporter_id = checkpoint_state.has("shadow_last_teleporter_id") ? String(checkpoint_state["shadow_last_teleporter_id"]) : String();

    replay_system->set_recorded_frames(recorded_frames);
    replay_system->set_recording(restore_recording);
    recording_active = restore_recording;
    record_toggle_requested = false;
    record_cancel_requested = false;
    recording_started_this_tick = false;
    recording_cancelled_this_tick = false;
    replay_started_this_tick = false;
    shadow_view = checkpoint_state.has("shadow_view") && (bool)checkpoint_state["shadow_view"];
    if (restore_recording) {
        player->set_recording_trail_points(player_trail_points);
        player->set_recording_trail_enabled(true);
    } else {
        player->clear_recording_trail();
        player->set_recording_trail_enabled(false);
    }
    player->set_velocity(player_velocity);
    if (player_last_teleporter_id.is_empty()) {
        player->remove_meta("last_teleporter_id");
    } else {
        player->set_meta("last_teleporter_id", player_last_teleporter_id);
    }

    if (restore_replaying) {
        shadow->restore_replay_state(shadow_replay_frames, replay_frame_index, shadow_position, shadow_velocity);
        _apply_input_frame_meta(shadow, shadow->peek_next_frame());
    } else {
        shadow->stop_replay();
        shadow->set_position(shadow_position);
        shadow->set_velocity(shadow_velocity);
        _clear_input_frame_meta(shadow);
    }
    if (shadow_last_teleporter_id.is_empty()) {
        shadow->remove_meta("last_teleporter_id");
    } else {
        shadow->set_meta("last_teleporter_id", shadow_last_teleporter_id);
    }

    _apply_camera_state();
}

bool MnmsPlayerShadowSystem::begin_recording() {
    MnmsReplaySystem *replay_system = _get_replay_system();
    MnmsShadow *shadow = _get_shadow();
    if (replay_system == nullptr || shadow == nullptr) {
        return false;
    }

    shadow->stop_replay();
    shadow->set_velocity(Vector2());
    _clear_input_frame_meta(shadow);
    MnmsPlayer *player = _get_player();
    if (player != nullptr) {
        player->clear_recording_trail();
        player->set_recording_trail_enabled(true);
    }
    replay_system->start_recording();
    recording_active = true;
    return true;
}

bool MnmsPlayerShadowSystem::cancel_recording() {
    MnmsReplaySystem *replay_system = _get_replay_system();
    MnmsShadow *shadow = _get_shadow();
    if (replay_system == nullptr || shadow == nullptr || !recording_active) {
        return false;
    }

    replay_system->stop_recording();
    replay_system->clear();
    recording_active = false;
    shadow->stop_replay();
    shadow->set_velocity(Vector2());
    _clear_input_frame_meta(shadow);
    MnmsPlayer *player = _get_player();
    if (player != nullptr) {
        player->clear_recording_trail();
        player->set_recording_trail_enabled(false);
    }
    return true;
}

bool MnmsPlayerShadowSystem::end_recording_and_replay() {
    MnmsReplaySystem *replay_system = _get_replay_system();
    MnmsShadow *shadow = _get_shadow();
    if (replay_system == nullptr || shadow == nullptr || !recording_active) {
        return false;
    }

    replay_system->stop_recording();
    recording_active = false;
    MnmsPlayer *player = _get_player();
    if (player != nullptr) {
        player->clear_recording_trail();
        player->set_recording_trail_enabled(false);
    }
    shadow->start_replay(replay_system->get_recorded_frames());
    return replay_system->get_frame_count() > 0;
}

bool MnmsPlayerShadowSystem::restart_replay() {
    MnmsReplaySystem *replay_system = _get_replay_system();
    MnmsPlayer *player = _get_player();
    MnmsShadow *shadow = _get_shadow();
    if (replay_system == nullptr || shadow == nullptr || replay_system->get_frame_count() <= 0) {
        return false;
    }

    recording_active = false;
    record_toggle_requested = false;
    record_cancel_requested = false;
    replay_system->stop_recording();
    shadow->stop_replay();
    shadow->reset_to_spawn();
    shadow->set_velocity(Vector2());
    _clear_input_frame_meta(shadow);
    if (player != nullptr) {
        player->clear_recording_trail();
        player->set_recording_trail_enabled(false);
    }
    shadow->start_replay(replay_system->get_recorded_frames());
    return shadow->is_replaying();
}

bool MnmsPlayerShadowSystem::stop_replay_playback() {
    MnmsPlayer *player = _get_player();
    MnmsShadow *shadow = _get_shadow();
    if (shadow == nullptr || !shadow->is_replaying()) {
        return false;
    }

    recording_active = false;
    MnmsReplaySystem *replay_system = _get_replay_system();
    if (replay_system != nullptr) {
        replay_system->stop_recording();
    }
    shadow->stop_replay();
    shadow->reset_to_spawn();
    shadow->set_velocity(Vector2());
    _clear_input_frame_meta(shadow);
    if (player != nullptr) {
        player->clear_recording_trail();
        player->set_recording_trail_enabled(false);
    }
    return true;
}

void MnmsPlayerShadowSystem::stop_replay() {
    recording_active = false;

    MnmsReplaySystem *replay_system = _get_replay_system();
    if (replay_system != nullptr) {
        replay_system->stop_recording();
        replay_system->clear();
    }

    MnmsShadow *shadow = _get_shadow();
    if (shadow != nullptr) {
        shadow->stop_replay();
        shadow->reset_to_spawn();
    }

    _clear_input_frame_meta(_get_shadow());
    MnmsPlayer *player = _get_player();
    if (player != nullptr) {
        player->clear_recording_trail();
        player->set_recording_trail_enabled(false);
    }
}

void MnmsPlayerShadowSystem::capture_frame() {
    MnmsReplaySystem *replay_system = _get_replay_system();
    if (!recording_active || replay_system == nullptr || !replay_system->is_recording()) {
        return;
    }

    replay_system->push_frame(_build_live_input_frame());
    MnmsPlayer *player = _get_player();
    if (player != nullptr) {
        player->append_recording_trail_sample();
    }
}

void MnmsPlayerShadowSystem::toggle_shadow_view() {
    shadow_view = !shadow_view;
    _apply_camera_state();
}

bool MnmsPlayerShadowSystem::is_shadow_view() const {
    return shadow_view;
}

bool MnmsPlayerShadowSystem::is_recording() const {
    return recording_active;
}

bool MnmsPlayerShadowSystem::is_replaying() const {
    MnmsShadow *shadow = _get_shadow();
    return shadow != nullptr && shadow->is_replaying();
}

bool MnmsPlayerShadowSystem::has_replay() const {
    MnmsReplaySystem *replay_system = _get_replay_system();
    return replay_system != nullptr && replay_system->get_frame_count() > 0;
}

bool MnmsPlayerShadowSystem::consume_recording_started() {
    const bool value = recording_started_this_tick;
    recording_started_this_tick = false;
    return value;
}

bool MnmsPlayerShadowSystem::consume_recording_cancelled() {
    const bool value = recording_cancelled_this_tick;
    recording_cancelled_this_tick = false;
    return value;
}

bool MnmsPlayerShadowSystem::consume_replay_started() {
    const bool value = replay_started_this_tick;
    replay_started_this_tick = false;
    return value;
}

void MnmsPlayerShadowSystem::_physics_process(double delta) {
    (void)delta;
    recording_started_this_tick = false;
    recording_cancelled_this_tick = false;
    replay_started_this_tick = false;

    if (record_cancel_requested) {
        if (cancel_recording()) {
            recording_cancelled_this_tick = true;
            record_toggle_requested = false;
        }
        record_cancel_requested = false;
    }

    if (record_toggle_requested) {
        MnmsShadow *shadow = _get_shadow();
        if (recording_active) {
            if (end_recording_and_replay()) {
                replay_started_this_tick = true;
            }
        } else if (shadow != nullptr && shadow->is_replaying()) {
            shadow->stop_replay();
            shadow->set_velocity(Vector2());
            _clear_input_frame_meta(shadow);
            if (begin_recording()) {
                recording_started_this_tick = true;
            }
        } else if (begin_recording()) {
            recording_started_this_tick = true;
        }
        record_toggle_requested = false;
    }

    MnmsPlayer *player = _get_player();
    if (player != nullptr) {
        _apply_input_frame_meta(player, _build_live_input_frame());
    }

    MnmsShadow *shadow = _get_shadow();
    if (shadow != nullptr && shadow->is_replaying()) {
        _apply_input_frame_meta(shadow, shadow->peek_next_frame());
    } else {
        _clear_input_frame_meta(shadow);
    }

    capture_frame();
}

MnmsPlayer *MnmsPlayerShadowSystem::_get_player() const {
    return Object::cast_to<MnmsPlayer>(get_node_or_null(player_path));
}

MnmsShadow *MnmsPlayerShadowSystem::_get_shadow() const {
    return Object::cast_to<MnmsShadow>(get_node_or_null(shadow_path));
}

MnmsReplaySystem *MnmsPlayerShadowSystem::_get_replay_system() const {
    return Object::cast_to<MnmsReplaySystem>(get_node_or_null(replay_system_path));
}

Dictionary MnmsPlayerShadowSystem::_build_live_input_frame() const {
    Input *input = Input::get_singleton();
    Dictionary frame;
    int move_dir = 0;
    if (input->is_action_pressed("ui_left")) {
        move_dir -= 1;
    }
    if (input->is_action_pressed("ui_right")) {
        move_dir += 1;
    }

    frame["move_dir"] = CLAMP(move_dir, -1, 1);
    frame["jump_pressed"] = input->is_action_just_pressed("ui_up");
    frame["jump_down"] = input->is_action_pressed("ui_up");
    frame["action_pressed"] = input->is_action_just_pressed("ui_down");
    frame["action_down"] = input->is_action_pressed("ui_down");
    return frame;
}

void MnmsPlayerShadowSystem::_apply_input_frame_meta(Node *p_target, const Dictionary &p_frame) const {
    if (p_target == nullptr) {
        return;
    }

    const int move_dir = p_frame.has("move_dir") ? (int)p_frame["move_dir"] : 0;
    const bool jump_pressed = p_frame.has("jump_pressed") && (bool)p_frame["jump_pressed"];
    const bool jump_down = p_frame.has("jump_down") && (bool)p_frame["jump_down"];
    const bool action_pressed = p_frame.has("action_pressed") && (bool)p_frame["action_pressed"];
    const bool action_down = p_frame.has("action_down") && (bool)p_frame["action_down"];

    p_target->set_meta("mnms_input_frame", p_frame);
    p_target->set_meta("mnms_move_dir", move_dir);
    p_target->set_meta("mnms_jump_pressed", jump_pressed);
    p_target->set_meta("mnms_jump_down", jump_down);
    p_target->set_meta("mnms_action_pressed", action_pressed);
    p_target->set_meta("mnms_action_down", action_down);
}

void MnmsPlayerShadowSystem::_clear_input_frame_meta(Node *p_target) const {
    if (p_target == nullptr) {
        return;
    }

    p_target->set_meta("mnms_input_frame", Dictionary());
    p_target->set_meta("mnms_move_dir", 0);
    p_target->set_meta("mnms_jump_pressed", false);
    p_target->set_meta("mnms_jump_down", false);
    p_target->set_meta("mnms_action_pressed", false);
    p_target->set_meta("mnms_action_down", false);
}

void MnmsPlayerShadowSystem::_apply_camera_state() {
    MnmsPlayer *player = _get_player();
    MnmsShadow *shadow = _get_shadow();
    Camera2D *player_camera = nullptr;
    Camera2D *shadow_camera = nullptr;

    if (player != nullptr) {
        player_camera = Object::cast_to<Camera2D>(player->get_node_or_null("PlayerCamera"));
    }
    if (shadow != nullptr) {
        shadow_camera = Object::cast_to<Camera2D>(shadow->get_node_or_null("ShadowCamera"));
    }

    if (player_camera != nullptr) {
        player_camera->set_enabled(!shadow_view);
    }
    if (shadow_camera != nullptr) {
        shadow_camera->set_enabled(shadow_view);
    }

    if (shadow_view) {
        if (shadow_camera != nullptr) {
            shadow_camera->make_current();
        }
    } else if (player_camera != nullptr) {
        player_camera->make_current();
    }
}
