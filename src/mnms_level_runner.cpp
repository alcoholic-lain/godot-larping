// src/mnms_level_runner.cpp
#include "mnms_level_runner.h"

#include "mnms_tile_resource.h"

#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/classes/camera2d.hpp>
#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/rigid_body2d.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace {

static const Color CHECKPOINT_DEFAULT_COLOR(0.95, 0.86, 0.27, 1.0);
static const Color CHECKPOINT_ACTIVE_COLOR(0.35, 1.0, 0.45, 1.0);

} // namespace

void MnmsLevelRunner::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_theme", "theme"), &MnmsLevelRunner::set_theme);
    ClassDB::bind_method(D_METHOD("get_theme"), &MnmsLevelRunner::get_theme);
    ClassDB::bind_method(D_METHOD("load_level", "level"), &MnmsLevelRunner::load_level);
    ClassDB::bind_method(D_METHOD("get_current_level"), &MnmsLevelRunner::get_current_level);
    ClassDB::bind_method(D_METHOD("restart_level"), &MnmsLevelRunner::restart_level);
    ClassDB::bind_method(D_METHOD("load_checkpoint"), &MnmsLevelRunner::load_checkpoint);
    ClassDB::bind_method(D_METHOD("has_checkpoint"), &MnmsLevelRunner::has_checkpoint);
    ClassDB::bind_method(D_METHOD("increment_recordings_used"), &MnmsLevelRunner::increment_recordings_used);
    ClassDB::bind_method(D_METHOD("set_recordings_used", "recordings_used"), &MnmsLevelRunner::set_recordings_used);
    ClassDB::bind_method(D_METHOD("get_recordings_used"), &MnmsLevelRunner::get_recordings_used);
    ClassDB::bind_method(D_METHOD("get_elapsed_ticks"), &MnmsLevelRunner::get_elapsed_ticks);
    ClassDB::bind_method(D_METHOD("get_target_time"), &MnmsLevelRunner::get_target_time);
    ClassDB::bind_method(D_METHOD("get_target_recordings"), &MnmsLevelRunner::get_target_recordings);
    ClassDB::bind_method(D_METHOD("get_current_notification_message"), &MnmsLevelRunner::get_current_notification_message);
    ClassDB::bind_method(D_METHOD("_on_spikes_body_entered", "body"), &MnmsLevelRunner::_on_spikes_body_entered);
    ClassDB::bind_method(D_METHOD("_on_shadow_spikes_body_entered", "body"), &MnmsLevelRunner::_on_shadow_spikes_body_entered);
    ClassDB::bind_method(D_METHOD("_on_exit_body_entered", "body"), &MnmsLevelRunner::_on_exit_body_entered);
    ClassDB::bind_method(D_METHOD("_on_checkpoint_body_entered", "body", "checkpoint_node"), &MnmsLevelRunner::_on_checkpoint_body_entered);
    ClassDB::bind_method(D_METHOD("_on_checkpoint_body_exited", "body", "checkpoint_node"), &MnmsLevelRunner::_on_checkpoint_body_exited);
    ClassDB::bind_method(D_METHOD("_on_collectable_body_entered", "body", "collectable_node"), &MnmsLevelRunner::_on_collectable_body_entered);
    ClassDB::bind_method(D_METHOD("_on_fragile_body_entered", "body", "fragile_node"), &MnmsLevelRunner::_on_fragile_body_entered);
    ClassDB::bind_method(D_METHOD("_on_fragile_body_exited", "body", "fragile_node"), &MnmsLevelRunner::_on_fragile_body_exited);
    ClassDB::bind_method(D_METHOD("_on_swap_body_entered", "body", "swap_node"), &MnmsLevelRunner::_on_swap_body_entered);
    ClassDB::bind_method(D_METHOD("_on_swap_body_exited", "body", "swap_node"), &MnmsLevelRunner::_on_swap_body_exited);
    ClassDB::bind_method(D_METHOD("_on_teleporter_body_entered", "body", "teleporter_node"), &MnmsLevelRunner::_on_teleporter_body_entered);
    ClassDB::bind_method(D_METHOD("_on_teleporter_body_exited", "body", "teleporter_node"), &MnmsLevelRunner::_on_teleporter_body_exited);
    ClassDB::bind_method(D_METHOD("_on_switch_body_entered", "body", "switch_node"), &MnmsLevelRunner::_on_switch_body_entered);
    ClassDB::bind_method(D_METHOD("_on_switch_body_exited", "body", "switch_node"), &MnmsLevelRunner::_on_switch_body_exited);
    ClassDB::bind_method(D_METHOD("_on_button_body_entered", "body", "button_node"), &MnmsLevelRunner::_on_button_body_entered);
    ClassDB::bind_method(D_METHOD("_on_button_body_exited", "body", "button_node"), &MnmsLevelRunner::_on_button_body_exited);
    ClassDB::bind_method(D_METHOD("_on_platform_body_entered", "body", "platform_node"), &MnmsLevelRunner::_on_platform_body_entered);
    ClassDB::bind_method(D_METHOD("_on_platform_body_exited", "body", "platform_node"), &MnmsLevelRunner::_on_platform_body_exited);
    ClassDB::bind_method(D_METHOD("_on_conveyor_body_entered", "body", "conveyor_node"), &MnmsLevelRunner::_on_conveyor_body_entered);
    ClassDB::bind_method(D_METHOD("_on_conveyor_body_exited", "body", "conveyor_node"), &MnmsLevelRunner::_on_conveyor_body_exited);
    ClassDB::bind_method(D_METHOD("_on_notification_body_entered", "body", "notification_node"), &MnmsLevelRunner::_on_notification_body_entered);
    ClassDB::bind_method(D_METHOD("_on_notification_body_exited", "body", "notification_node"), &MnmsLevelRunner::_on_notification_body_exited);
    ADD_SIGNAL(MethodInfo("level_completed", PropertyInfo(Variant::STRING, "level_name")));
    ADD_SIGNAL(MethodInfo("level_failed"));
    ADD_SIGNAL(MethodInfo("checkpoint_activated"));
    ADD_SIGNAL(MethodInfo("collectable_collected"));
    ADD_SIGNAL(MethodInfo("swap_activated"));
    ADD_SIGNAL(MethodInfo("runtime_state_changed",
        PropertyInfo(Variant::INT, "elapsed_ticks"),
        PropertyInfo(Variant::INT, "recordings_used"),
        PropertyInfo(Variant::INT, "target_time"),
        PropertyInfo(Variant::INT, "target_recordings")));
    ADD_SIGNAL(MethodInfo("notification_changed", PropertyInfo(Variant::STRING, "message")));

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "theme", PROPERTY_HINT_RESOURCE_TYPE, "MnmsThemeResource"), "set_theme", "get_theme");
}

MnmsLevelRunner::MnmsLevelRunner() {
    block_system = nullptr;
    tile_layer = nullptr;
    player = nullptr;
    shadow = nullptr;
    checkpoint_active = false;
    level_complete_pending = false;
    level_failed_pending = false;
    player_spawn = Vector2(120, 120);
    shadow_spawn = Vector2(180, 120);
    checkpoint_player_position = player_spawn;
    checkpoint_shadow_position = shadow_spawn;
    elapsed_ticks = 0;
    recordings_used = 0;
    checkpoint_elapsed_ticks = 0;
    checkpoint_recordings_used = 0;
    total_collectables = 0;
    collected_collectables = 0;
    checkpoint_runtime_state.clear();
    set_physics_process(true);
}

MnmsLevelRunner::~MnmsLevelRunner() {}

void MnmsLevelRunner::_ready() {
    tile_layer = memnew(Node2D);
    tile_layer->set_name("TileLayer");
    add_child(tile_layer);

    block_system = memnew(MnmsBlockSystem);
    block_system->set_name("BlockSystem");
    add_child(block_system);
}

void MnmsLevelRunner::_physics_process(double delta) {
    elapsed_ticks += 1;
    _update_moving_nodes(delta);
    _update_conveyors(delta);
    _update_fragile_blocks(delta);
    _update_button_states();
    _try_activate_checkpoints();
    _try_activate_teleporters();
    _try_activate_switches();
    _try_activate_swap();
    _emit_runtime_state_changed();
}

void MnmsLevelRunner::set_theme(const Ref<MnmsThemeResource> &p_theme) {
    theme = p_theme;
    if (block_system != nullptr) {
        block_system->set_theme(theme);
    }
}

Ref<MnmsThemeResource> MnmsLevelRunner::get_theme() const {
    return theme;
}

void MnmsLevelRunner::_clear_level() {
    if (tile_layer) {
        Array children = tile_layer->get_children();
        for (int i = 0; i < children.size(); i++) {
            Node *node = Object::cast_to<Node>(children[i]);
            if (node) {
                node->queue_free();
            }
        }
    }
    if (player) {
        player->queue_free();
        player = nullptr;
    }
    if (shadow) {
        shadow->queue_free();
        shadow = nullptr;
    }
}

void MnmsLevelRunner::_spawn_player_shadow() {
    player = memnew(MnmsPlayer);
    player->set_name("Player");
    if (theme.is_valid()) {
        Dictionary character_states = theme->get_character_state_map();
        if (character_states.has("Player")) {
            player->set_character_states(character_states["Player"]);
        }
    }
    player->set_position(player_spawn);
    add_child(player);

    Camera2D *cam = memnew(Camera2D);
    cam->set_name("PlayerCamera");
    cam->set_enabled(true);
    player->add_child(cam);
    _configure_camera(cam);

    shadow = memnew(MnmsShadow);
    shadow->set_name("Shadow");
    if (theme.is_valid()) {
        Dictionary character_states = theme->get_character_state_map();
        if (character_states.has("Shadow")) {
            shadow->set_character_states(character_states["Shadow"]);
        }
    }
    add_child(shadow);
    shadow->set_spawn_position(shadow_spawn);
    shadow->reset_to_spawn();

    Camera2D *shadow_camera = memnew(Camera2D);
    shadow_camera->set_name("ShadowCamera");
    shadow_camera->set_enabled(false);
    shadow->add_child(shadow_camera);
    _configure_camera(shadow_camera);
}

void MnmsLevelRunner::_configure_camera(Camera2D *p_camera) const {
    if (p_camera == nullptr || current_level.is_null()) {
        return;
    }

    Rect2 viewport_rect = get_viewport_rect();
    Vector2 viewport_size = viewport_rect.size;
    if (viewport_size.x <= 0.0 || viewport_size.y <= 0.0) {
        viewport_size = Vector2(800.0, 600.0);
    }

    const Vector2i level_size_i = current_level->get_world_size();
    const int32_t level_width = MAX(level_size_i.x, 1);
    const int32_t level_height = MAX(level_size_i.y, 1);

    p_camera->set_limit_enabled(true);
    p_camera->set_position_smoothing_enabled(true);
    p_camera->set_position_smoothing_speed(8.0);
    p_camera->set_limit_smoothing_enabled(false);
    p_camera->set_drag_horizontal_enabled(true);
    p_camera->set_drag_vertical_enabled(true);
    p_camera->set_drag_margin(SIDE_LEFT, 0.4375);
    p_camera->set_drag_margin(SIDE_RIGHT, 0.4375);
    p_camera->set_drag_margin(SIDE_TOP, 0.4166667);
    p_camera->set_drag_margin(SIDE_BOTTOM, 0.4166667);

    const int32_t half_view_width = (int32_t)Math::round(viewport_size.x * 0.5);
    const int32_t half_view_height = (int32_t)Math::round(viewport_size.y * 0.5);
    const int32_t limit_left = level_width <= (int32_t)viewport_size.x ? level_width / 2 : half_view_width;
    const int32_t limit_right = level_width <= (int32_t)viewport_size.x ? level_width / 2 : level_width - half_view_width;
    const int32_t limit_top = level_height <= (int32_t)viewport_size.y ? level_height - half_view_height : half_view_height;
    const int32_t limit_bottom = level_height <= (int32_t)viewport_size.y ? level_height - half_view_height : level_height - half_view_height;

    p_camera->set_limit(SIDE_LEFT, limit_left);
    p_camera->set_limit(SIDE_RIGHT, limit_right);
    p_camera->set_limit(SIDE_TOP, limit_top);
    p_camera->set_limit(SIDE_BOTTOM, limit_bottom);
    p_camera->reset_smoothing();
    p_camera->align();
    p_camera->force_update_scroll();
}

void MnmsLevelRunner::load_level(const Ref<MnmsLevelResource> &p_level) {
    if (p_level.is_null() || block_system == nullptr || tile_layer == nullptr) {
        return;
    }

    current_level = p_level;
    _clear_level();
    if (block_system != nullptr) {
        block_system->set_theme(theme);
    }
    checkpoint_active = false;
    level_complete_pending = false;
    level_failed_pending = false;

    player_spawn = Vector2(120, 120);
    shadow_spawn = Vector2(180, 120);
    checkpoint_player_position = player_spawn;
    checkpoint_shadow_position = shadow_spawn;
    elapsed_ticks = 0;
    recordings_used = 0;
    checkpoint_elapsed_ticks = 0;
    checkpoint_recordings_used = 0;
    total_collectables = 0;
    collected_collectables = 0;
    checkpoint_nodes.clear();
    collectable_nodes.clear();
    exit_nodes.clear();
    fragile_nodes.clear();
    swap_nodes.clear();
    teleporter_nodes.clear();
    switch_nodes.clear();
    button_nodes.clear();
    moving_nodes.clear();
    conveyor_nodes.clear();
    pushable_nodes.clear();
    controlled_nodes.clear();
    current_notification_message = "";
    checkpoint_runtime_state.clear();

    const Array tiles = p_level->get_tiles();
    for (int i = 0; i < tiles.size(); i++) {
        Ref<MnmsTileResource> tile = tiles[i];
        if (tile.is_null()) {
            continue;
        }

        const String block_type = tile->get_block_type();
        const Vector2 pos = Vector2(tile->get_position());
        const Vector2 size = Vector2(tile->get_size());

        if (block_type == "PlayerStart") {
            player_spawn = pos + size * 0.5;
            continue;
        }
        if (block_type == "ShadowStart") {
            shadow_spawn = pos + size * 0.5;
            continue;
        }

        Node *spawned = block_system->spawn_tile(tile, tile_layer);
        Area2D *area = Object::cast_to<Area2D>(spawned);
        if (area) {
            if (area->is_in_group("mnms_spikes")) {
                area->connect("body_entered", Callable(this, "_on_spikes_body_entered"));
            } else if (area->is_in_group("mnms_shadow_spikes")) {
                area->connect("body_entered", Callable(this, "_on_shadow_spikes_body_entered"));
            } else if (area->is_in_group("mnms_exit")) {
                area->connect("body_entered", Callable(this, "_on_exit_body_entered"));
                exit_nodes.push_back(area);
            } else if (area->is_in_group("mnms_checkpoint")) {
                area->connect("body_entered", Callable(this, "_on_checkpoint_body_entered").bind(area));
                area->connect("body_exited", Callable(this, "_on_checkpoint_body_exited").bind(area));
                area->set_meta("player_inside", false);
                area->set_meta("shadow_inside", false);
                checkpoint_nodes.push_back(area);
            } else if (area->is_in_group("mnms_collectable")) {
                area->connect("body_entered", Callable(this, "_on_collectable_body_entered").bind(area));
                total_collectables += 1;
                collectable_nodes.push_back(area);
            } else if (area->is_in_group("mnms_swap")) {
                area->connect("body_entered", Callable(this, "_on_swap_body_entered").bind(area));
                area->connect("body_exited", Callable(this, "_on_swap_body_exited").bind(area));
                swap_nodes.push_back(area);
            } else if (area->is_in_group("mnms_teleporter")) {
                area->connect("body_entered", Callable(this, "_on_teleporter_body_entered").bind(area));
                area->connect("body_exited", Callable(this, "_on_teleporter_body_exited").bind(area));
                teleporter_nodes.push_back(area);
            } else if (area->is_in_group("mnms_switch")) {
                area->connect("body_entered", Callable(this, "_on_switch_body_entered").bind(area));
                area->connect("body_exited", Callable(this, "_on_switch_body_exited").bind(area));
                switch_nodes.push_back(area);
            } else if (area->is_in_group("mnms_notification")) {
                area->connect("body_entered", Callable(this, "_on_notification_body_entered").bind(area));
                area->connect("body_exited", Callable(this, "_on_notification_body_exited").bind(area));
            }
        }

        if (spawned != nullptr && spawned->is_in_group("mnms_moving_platform_root")) {
            Node *detector_node = spawned->get_node_or_null("Detector");
            Area2D *detector = Object::cast_to<Area2D>(detector_node);
            if (detector != nullptr) {
                detector->connect("body_entered", Callable(this, "_on_platform_body_entered").bind(spawned));
                detector->connect("body_exited", Callable(this, "_on_platform_body_exited").bind(spawned));
            }
            moving_nodes.push_back(spawned);
        }
        if (spawned != nullptr && spawned->is_in_group("mnms_moving_hazard_root")) {
            moving_nodes.push_back(spawned);
        }
        if (spawned != nullptr && spawned->is_in_group("mnms_conveyor_root")) {
            Node *detector_node = spawned->get_node_or_null("Detector");
            Area2D *detector = Object::cast_to<Area2D>(detector_node);
            if (detector != nullptr) {
                detector->connect("body_entered", Callable(this, "_on_conveyor_body_entered").bind(spawned));
                detector->connect("body_exited", Callable(this, "_on_conveyor_body_exited").bind(spawned));
            }
            conveyor_nodes.push_back(spawned);
        }
        if (spawned != nullptr && (spawned->is_in_group("mnms_fragile_root") || spawned->is_in_group("mnms_shadow_fragile_root"))) {
            Node *detector_node = spawned->get_node_or_null("Detector");
            Area2D *detector = Object::cast_to<Area2D>(detector_node);
            if (detector != nullptr) {
                detector->connect("body_entered", Callable(this, "_on_fragile_body_entered").bind(spawned));
                detector->connect("body_exited", Callable(this, "_on_fragile_body_exited").bind(spawned));
                fragile_nodes.push_back(spawned);
            }
        }
        if (spawned != nullptr && spawned->is_in_group("mnms_button_root")) {
            Node *detector_node = spawned->get_node_or_null("Detector");
            Area2D *detector = Object::cast_to<Area2D>(detector_node);
            if (detector != nullptr) {
                detector->connect("body_entered", Callable(this, "_on_button_body_entered").bind(spawned));
                detector->connect("body_exited", Callable(this, "_on_button_body_exited").bind(spawned));
                button_nodes.push_back(spawned);
            }
        }
        if (spawned != nullptr && spawned->has_meta("control_id")) {
            controlled_nodes.push_back(spawned);
        }
        if (spawned != nullptr && (spawned->is_in_group("mnms_pushable") || spawned->is_in_group("mnms_shadow_pushable"))) {
            pushable_nodes.push_back(spawned);
        }
    }

    for (int i = 0; i < controlled_nodes.size(); i++) {
        Node *controlled_node = Object::cast_to<Node>(controlled_nodes[i]);
        if (controlled_node != nullptr && controlled_node->has_meta("enabled")) {
            _set_controlled_node_enabled(controlled_node, (bool)controlled_node->get_meta("enabled"));
        }
    }

    for (int i = 0; i < exit_nodes.size(); i++) {
        Node *exit_node = Object::cast_to<Node>(exit_nodes[i]);
        _refresh_exit_open_state(exit_node);
    }

    _spawn_player_shadow();
    _set_notification_message("");
    _emit_runtime_state_changed();
}

Ref<MnmsLevelResource> MnmsLevelRunner::get_current_level() const {
    return current_level;
}

void MnmsLevelRunner::restart_level() {
    if (current_level.is_valid()) {
        const bool preserve_checkpoint = checkpoint_active;
        const Vector2 saved_checkpoint_player_position = checkpoint_player_position;
        const Vector2 saved_checkpoint_shadow_position = checkpoint_shadow_position;
        const int32_t saved_checkpoint_elapsed_ticks = checkpoint_elapsed_ticks;
        const int32_t saved_checkpoint_recordings_used = checkpoint_recordings_used;
        const Dictionary saved_checkpoint_runtime_state = checkpoint_runtime_state.duplicate(true);

        load_level(current_level);

        if (preserve_checkpoint) {
            checkpoint_active = true;
            checkpoint_player_position = saved_checkpoint_player_position;
            checkpoint_shadow_position = saved_checkpoint_shadow_position;
            checkpoint_elapsed_ticks = saved_checkpoint_elapsed_ticks;
            checkpoint_recordings_used = saved_checkpoint_recordings_used;
            checkpoint_runtime_state = saved_checkpoint_runtime_state;

            if (checkpoint_runtime_state.has("active_checkpoint_path")) {
                Node *active_checkpoint = Object::cast_to<Node>(get_node_or_null(NodePath(checkpoint_runtime_state["active_checkpoint_path"])));
                _set_checkpoint_visual(active_checkpoint, CHECKPOINT_ACTIVE_COLOR);
            }
        }
    }
}

void MnmsLevelRunner::load_checkpoint() {
    if (!checkpoint_active || player == nullptr || shadow == nullptr) {
        return;
    }

    level_failed_pending = false;
    level_complete_pending = false;
    _restore_checkpoint_runtime_state();
    player->set_position(checkpoint_player_position);
    player->set_velocity(Vector2());
    player->remove_meta("last_teleporter_id");

    shadow->set_spawn_position(checkpoint_shadow_position);
    shadow->reset_to_spawn();
    shadow->set_velocity(Vector2());
    shadow->remove_meta("last_teleporter_id");
    level_complete_pending = false;
    elapsed_ticks = checkpoint_elapsed_ticks;
    recordings_used = checkpoint_recordings_used;
    _emit_runtime_state_changed();
}

bool MnmsLevelRunner::has_checkpoint() const {
    return checkpoint_active;
}

MnmsPlayer *MnmsLevelRunner::get_player() const {
    return player;
}

MnmsShadow *MnmsLevelRunner::get_shadow() const {
    return shadow;
}

Vector2 MnmsLevelRunner::get_player_spawn() const {
    return player_spawn;
}

Vector2 MnmsLevelRunner::get_shadow_spawn() const {
    return shadow_spawn;
}

void MnmsLevelRunner::increment_recordings_used() {
    recordings_used += 1;
    _emit_runtime_state_changed();
}

void MnmsLevelRunner::set_recordings_used(int32_t p_recordings_used) {
    recordings_used = MAX(0, p_recordings_used);
    _emit_runtime_state_changed();
}

int32_t MnmsLevelRunner::get_recordings_used() const {
    return recordings_used;
}

int32_t MnmsLevelRunner::get_elapsed_ticks() const {
    return elapsed_ticks;
}

int32_t MnmsLevelRunner::get_target_time() const {
    return current_level.is_valid() ? current_level->get_time_limit() : -1;
}

int32_t MnmsLevelRunner::get_target_recordings() const {
    return current_level.is_valid() ? current_level->get_recordings_count() : -1;
}

String MnmsLevelRunner::get_current_notification_message() const {
    return current_notification_message;
}

void MnmsLevelRunner::_on_spikes_body_entered(Node2D *p_body) {
    if (p_body != nullptr &&
        !level_complete_pending &&
        !level_failed_pending &&
        p_body->is_in_group("mnms_player") &&
        current_level.is_valid()) {
        level_failed_pending = true;
        emit_signal("level_failed");
    } else if (p_body != nullptr && p_body->is_in_group("mnms_shadow") && current_level.is_valid()) {
        _handle_shadow_failure();
    }
}

void MnmsLevelRunner::_on_shadow_spikes_body_entered(Node2D *p_body) {
    if (p_body != nullptr &&
        p_body->is_in_group("mnms_shadow") &&
        current_level.is_valid()) {
        _handle_shadow_failure();
    }
}

void MnmsLevelRunner::_on_exit_body_entered(Node2D *p_body) {
    if (p_body != nullptr &&
        elapsed_ticks > 2 &&
        _is_completion_body(p_body) &&
        !level_complete_pending &&
        !level_failed_pending) {
        level_complete_pending = true;
        UtilityFunctions::print("MNMS: niveau complete -> ", current_level->get_level_name());
        emit_signal("level_completed", current_level->get_level_name());
    }
}

void MnmsLevelRunner::_on_checkpoint_body_entered(Node2D *p_body, Node *p_checkpoint_node) {
    if (p_body == nullptr || p_checkpoint_node == nullptr || player == nullptr || shadow == nullptr) {
        return;
    }
    if (p_body->is_in_group("mnms_player")) {
        p_checkpoint_node->set_meta("player_inside", true);
    } else if (p_body->is_in_group("mnms_shadow")) {
        p_checkpoint_node->set_meta("shadow_inside", true);
    }
    _set_notification_message("Action: sauvegarder au checkpoint.");
}

void MnmsLevelRunner::_on_checkpoint_body_exited(Node2D *p_body, Node *p_checkpoint_node) {
    if (p_body == nullptr || p_checkpoint_node == nullptr) {
        return;
    }
    if (p_body->is_in_group("mnms_player")) {
        p_checkpoint_node->set_meta("player_inside", false);
    } else if (p_body->is_in_group("mnms_shadow")) {
        p_checkpoint_node->set_meta("shadow_inside", false);
    }
    if (current_notification_message == "Action: sauvegarder au checkpoint.") {
        _set_notification_message("");
    }
}

void MnmsLevelRunner::_on_collectable_body_entered(Node2D *p_body, Node *p_collectable_node) {
    if (p_body == nullptr || p_collectable_node == nullptr || (!p_body->is_in_group("mnms_player") && !p_body->is_in_group("mnms_shadow"))) {
        return;
    }

    if (p_collectable_node->has_meta("collected") && (bool)p_collectable_node->get_meta("collected")) {
        return;
    }

    p_collectable_node->set_meta("collected", true);
    collected_collectables += 1;

    Area2D *collectable = Object::cast_to<Area2D>(p_collectable_node);
    if (collectable != nullptr) {
        collectable->set_deferred("monitoring", false);
    }

    CanvasItem *visual = Object::cast_to<CanvasItem>(p_collectable_node->get_node_or_null("Visual"));
    if (visual != nullptr) {
        visual->set_visible(false);
    }

    if (collected_collectables >= total_collectables) {
        for (int i = 0; i < exit_nodes.size(); i++) {
            Node *exit_node = Object::cast_to<Node>(exit_nodes[i]);
            _refresh_exit_open_state(exit_node);
        }
    }
    emit_signal("collectable_collected");
}

void MnmsLevelRunner::_on_fragile_body_entered(Node2D *p_body, Node *p_fragile_node) {
    (void)p_body;
    (void)p_fragile_node;
}

void MnmsLevelRunner::_on_fragile_body_exited(Node2D *p_body, Node *p_fragile_node) {
    (void)p_body;
    (void)p_fragile_node;
}

void MnmsLevelRunner::_on_swap_body_entered(Node2D *p_body, Node *p_swap_node) {
    if (p_body == nullptr || p_swap_node == nullptr) {
        return;
    }
    if (p_body->is_in_group("mnms_player")) {
        p_swap_node->set_meta("player_inside", true);
    } else if (p_body->is_in_group("mnms_shadow")) {
        p_swap_node->set_meta("shadow_inside", true);
    }
    _set_notification_message("Action: echanger player et shadow.");
}

void MnmsLevelRunner::_on_swap_body_exited(Node2D *p_body, Node *p_swap_node) {
    if (p_body == nullptr || p_swap_node == nullptr) {
        return;
    }
    if (p_body->is_in_group("mnms_player")) {
        p_swap_node->set_meta("player_inside", false);
    } else if (p_body->is_in_group("mnms_shadow")) {
        p_swap_node->set_meta("shadow_inside", false);
    }
    if (current_notification_message == "Action: echanger player et shadow.") {
        _set_notification_message("");
    }
}

void MnmsLevelRunner::_on_teleporter_body_entered(Node2D *p_body, Node *p_teleporter_node) {
    if (p_body == nullptr || p_teleporter_node == nullptr) {
        return;
    }
    if (p_body->is_in_group("mnms_player")) {
        p_teleporter_node->set_meta("player_inside", true);
    } else if (p_body->is_in_group("mnms_shadow")) {
        p_teleporter_node->set_meta("shadow_inside", true);
    }
    const bool automatic = p_teleporter_node->has_meta("automatic") && (bool)p_teleporter_node->get_meta("automatic");
    if (!automatic) {
        if (p_teleporter_node->has_meta("message")) {
            _set_notification_message(String(p_teleporter_node->get_meta("message")));
        } else {
            _set_notification_message("Action: utiliser le teleporteur.");
        }
    }
}

void MnmsLevelRunner::_on_teleporter_body_exited(Node2D *p_body, Node *p_teleporter_node) {
    if (p_body == nullptr || p_teleporter_node == nullptr) {
        return;
    }
    if (p_body->is_in_group("mnms_player")) {
        p_teleporter_node->set_meta("player_inside", false);
    } else if (p_body->is_in_group("mnms_shadow")) {
        p_teleporter_node->set_meta("shadow_inside", false);
    }

    if (p_body->has_meta("last_teleporter_id") && String(p_body->get_meta("last_teleporter_id")) == String::num_int64(p_teleporter_node->get_instance_id())) {
        p_body->remove_meta("last_teleporter_id");
    }
    const String teleporter_message = p_teleporter_node->has_meta("message")
        ? String(p_teleporter_node->get_meta("message"))
        : String("Action: utiliser le teleporteur.");
    if (current_notification_message == teleporter_message) {
        _set_notification_message("");
    }
}

void MnmsLevelRunner::_on_switch_body_entered(Node2D *p_body, Node *p_switch_node) {
    if (p_body == nullptr || p_switch_node == nullptr) {
        return;
    }
    if (p_body->is_in_group("mnms_player")) {
        p_switch_node->set_meta("player_inside", true);
    } else if (p_body->is_in_group("mnms_shadow")) {
        p_switch_node->set_meta("shadow_inside", true);
    }
    if (p_switch_node->has_meta("message")) {
        _set_notification_message(String(p_switch_node->get_meta("message")));
    } else {
        _set_notification_message("Action: activer l'interrupteur.");
    }
}

void MnmsLevelRunner::_on_switch_body_exited(Node2D *p_body, Node *p_switch_node) {
    if (p_body == nullptr || p_switch_node == nullptr) {
        return;
    }
    if (p_body->is_in_group("mnms_player")) {
        p_switch_node->set_meta("player_inside", false);
    } else if (p_body->is_in_group("mnms_shadow")) {
        p_switch_node->set_meta("shadow_inside", false);
    }
    const String switch_message = p_switch_node->has_meta("message")
        ? String(p_switch_node->get_meta("message"))
        : String("Action: activer l'interrupteur.");
    if (current_notification_message == switch_message) {
        _set_notification_message("");
    }
}

void MnmsLevelRunner::_on_button_body_entered(Node2D *p_body, Node *p_button_node) {
    (void)p_body;
    (void)p_button_node;
}

void MnmsLevelRunner::_on_button_body_exited(Node2D *p_body, Node *p_button_node) {
    (void)p_body;
    (void)p_button_node;
}

void MnmsLevelRunner::_on_platform_body_entered(Node2D *p_body, Node *p_platform_node) {
    (void)p_body;
    (void)p_platform_node;
}

void MnmsLevelRunner::_on_platform_body_exited(Node2D *p_body, Node *p_platform_node) {
    (void)p_body;
    (void)p_platform_node;
}

void MnmsLevelRunner::_on_conveyor_body_entered(Node2D *p_body, Node *p_conveyor_node) {
    (void)p_body;
    (void)p_conveyor_node;
}

void MnmsLevelRunner::_on_conveyor_body_exited(Node2D *p_body, Node *p_conveyor_node) {
    (void)p_body;
    (void)p_conveyor_node;
}

void MnmsLevelRunner::_on_notification_body_entered(Node2D *p_body, Node *p_notification_node) {
    if (p_body == nullptr || p_notification_node == nullptr) {
        return;
    }
    if (!p_body->is_in_group("mnms_player") && !p_body->is_in_group("mnms_shadow")) {
        return;
    }
    if (!p_notification_node->has_meta("message")) {
        return;
    }
    _set_notification_message(String(p_notification_node->get_meta("message")));
}

void MnmsLevelRunner::_on_notification_body_exited(Node2D *p_body, Node *p_notification_node) {
    if (p_body == nullptr || p_notification_node == nullptr) {
        return;
    }
    if (!p_body->is_in_group("mnms_player") && !p_body->is_in_group("mnms_shadow")) {
        return;
    }
    if (!p_notification_node->has_meta("message")) {
        return;
    }
    if (current_notification_message == String(p_notification_node->get_meta("message"))) {
        _set_notification_message("");
    }
}

void MnmsLevelRunner::_activate_checkpoint(const Vector2 &p_player_position, const Vector2 &p_shadow_position, Node *p_checkpoint_node) {
    checkpoint_active = true;
    checkpoint_player_position = p_player_position;
    checkpoint_shadow_position = p_shadow_position;
    checkpoint_elapsed_ticks = elapsed_ticks;
    checkpoint_recordings_used = recordings_used;
    _capture_checkpoint_runtime_state(p_checkpoint_node);
    shadow->set_spawn_position(checkpoint_shadow_position);
    _emit_runtime_state_changed();
    emit_signal("checkpoint_activated");
}

void MnmsLevelRunner::_capture_checkpoint_runtime_state(Node *p_checkpoint_node) {
    Dictionary state;
    state["active_checkpoint_path"] = p_checkpoint_node != nullptr ? Variant(get_path_to(p_checkpoint_node)) : Variant(NodePath());
    state["collected_collectables"] = collected_collectables;
    state["notification_message"] = current_notification_message;

    Array collectable_states;
    for (int i = 0; i < collectable_nodes.size(); i++) {
        Node *collectable_node = Object::cast_to<Node>(collectable_nodes[i]);
        if (collectable_node == nullptr || collectable_node->is_queued_for_deletion()) {
            continue;
        }

        Dictionary collectable_state;
        collectable_state["path"] = get_path_to(collectable_node);
        collectable_state["collected"] = collectable_node->has_meta("collected") && (bool)collectable_node->get_meta("collected");
        collectable_states.push_back(collectable_state);
    }
    state["collectables"] = collectable_states;

    Array fragile_states;
    for (int i = 0; i < fragile_nodes.size(); i++) {
        Node *fragile_node = Object::cast_to<Node>(fragile_nodes[i]);
        if (fragile_node == nullptr || fragile_node->is_queued_for_deletion()) {
            continue;
        }

        Dictionary fragile_state;
        fragile_state["path"] = get_path_to(fragile_node);
        fragile_state["fragile_broken"] = fragile_node->has_meta("fragile_broken") && (bool)fragile_node->get_meta("fragile_broken");
        fragile_state["fragile_stage"] = fragile_node->has_meta("fragile_stage") ? fragile_node->get_meta("fragile_stage") : Variant(0);
        fragile_state["fragile_progress"] = fragile_node->has_meta("fragile_progress") ? fragile_node->get_meta("fragile_progress") : Variant(0.0);
        fragile_state["fragile_active_bodies"] = fragile_node->has_meta("fragile_active_bodies") ? fragile_node->get_meta("fragile_active_bodies") : Variant(0);
        fragile_states.push_back(fragile_state);
    }
    state["fragile_nodes"] = fragile_states;

    Array swap_states;
    for (int i = 0; i < swap_nodes.size(); i++) {
        Node *swap_node = Object::cast_to<Node>(swap_nodes[i]);
        if (swap_node == nullptr || swap_node->is_queued_for_deletion()) {
            continue;
        }

        Dictionary swap_state;
        swap_state["path"] = get_path_to(swap_node);
        swap_state["player_inside"] = swap_node->has_meta("player_inside") && (bool)swap_node->get_meta("player_inside");
        swap_state["shadow_inside"] = swap_node->has_meta("shadow_inside") && (bool)swap_node->get_meta("shadow_inside");
        swap_states.push_back(swap_state);
    }
    state["swap_nodes"] = swap_states;

    Array teleporter_states;
    for (int i = 0; i < teleporter_nodes.size(); i++) {
        Node *teleporter_node = Object::cast_to<Node>(teleporter_nodes[i]);
        if (teleporter_node == nullptr || teleporter_node->is_queued_for_deletion()) {
            continue;
        }

        Dictionary teleporter_state;
        teleporter_state["path"] = get_path_to(teleporter_node);
        teleporter_state["player_inside"] = teleporter_node->has_meta("player_inside") && (bool)teleporter_node->get_meta("player_inside");
        teleporter_state["shadow_inside"] = teleporter_node->has_meta("shadow_inside") && (bool)teleporter_node->get_meta("shadow_inside");
        teleporter_states.push_back(teleporter_state);
    }
    state["teleporter_nodes"] = teleporter_states;

    Array switch_states;
    for (int i = 0; i < switch_nodes.size(); i++) {
        Node *switch_node = Object::cast_to<Node>(switch_nodes[i]);
        if (switch_node == nullptr || switch_node->is_queued_for_deletion()) {
            continue;
        }

        Dictionary switch_state;
        switch_state["path"] = get_path_to(switch_node);
        switch_state["player_inside"] = switch_node->has_meta("player_inside") && (bool)switch_node->get_meta("player_inside");
        switch_state["shadow_inside"] = switch_node->has_meta("shadow_inside") && (bool)switch_node->get_meta("shadow_inside");
        switch_states.push_back(switch_state);
    }
    state["switch_nodes"] = switch_states;

    Array button_states;
    for (int i = 0; i < button_nodes.size(); i++) {
        Node *button_node = Object::cast_to<Node>(button_nodes[i]);
        if (button_node == nullptr || button_node->is_queued_for_deletion()) {
            continue;
        }

        Dictionary button_state;
        button_state["path"] = get_path_to(button_node);
        button_state["active_bodies"] = button_node->has_meta("active_bodies") ? button_node->get_meta("active_bodies") : Variant(0);
        button_state["button_pressed"] = button_node->has_meta("button_pressed") && (bool)button_node->get_meta("button_pressed");
        button_states.push_back(button_state);
    }
    state["button_nodes"] = button_states;

    Array moving_states;
    for (int i = 0; i < moving_nodes.size(); i++) {
        Node *moving_node = Object::cast_to<Node>(moving_nodes[i]);
        Node2D *moving_node_2d = Object::cast_to<Node2D>(moving_node);
        if (moving_node == nullptr || moving_node_2d == nullptr || moving_node->is_queued_for_deletion()) {
            continue;
        }

        Dictionary moving_state;
        moving_state["path"] = get_path_to(moving_node);
        moving_state["position"] = moving_node_2d->get_position();
        moving_state["moving_time"] = moving_node->has_meta("moving_time") ? moving_node->get_meta("moving_time") : Variant(0.0);
        moving_state["player_inside"] = moving_node->has_meta("player_inside") && (bool)moving_node->get_meta("player_inside");
        moving_state["shadow_inside"] = moving_node->has_meta("shadow_inside") && (bool)moving_node->get_meta("shadow_inside");
        moving_states.push_back(moving_state);
    }
    state["moving_nodes"] = moving_states;

    Array conveyor_states;
    for (int i = 0; i < conveyor_nodes.size(); i++) {
        Node *conveyor_node = Object::cast_to<Node>(conveyor_nodes[i]);
        if (conveyor_node == nullptr || conveyor_node->is_queued_for_deletion()) {
            continue;
        }

        Dictionary conveyor_state;
        conveyor_state["path"] = get_path_to(conveyor_node);
        conveyor_state["player_inside"] = conveyor_node->has_meta("player_inside") && (bool)conveyor_node->get_meta("player_inside");
        conveyor_state["shadow_inside"] = conveyor_node->has_meta("shadow_inside") && (bool)conveyor_node->get_meta("shadow_inside");
        conveyor_states.push_back(conveyor_state);
    }
    state["conveyor_nodes"] = conveyor_states;

    Array pushable_states;
    for (int i = 0; i < pushable_nodes.size(); i++) {
        Node *pushable_node = Object::cast_to<Node>(pushable_nodes[i]);
        RigidBody2D *pushable_body = Object::cast_to<RigidBody2D>(pushable_node);
        if (pushable_node == nullptr || pushable_body == nullptr || pushable_node->is_queued_for_deletion()) {
            continue;
        }

        Dictionary pushable_state;
        pushable_state["path"] = get_path_to(pushable_node);
        pushable_state["position"] = pushable_body->get_position();
        pushable_state["linear_velocity"] = pushable_body->get_linear_velocity();
        pushable_state["angular_velocity"] = pushable_body->get_angular_velocity();
        pushable_states.push_back(pushable_state);
    }
    state["pushable_nodes"] = pushable_states;

    Array controlled_states;
    for (int i = 0; i < controlled_nodes.size(); i++) {
        Node *controlled_node = Object::cast_to<Node>(controlled_nodes[i]);
        if (controlled_node == nullptr || controlled_node->is_queued_for_deletion()) {
            continue;
        }

        Dictionary controlled_state;
        controlled_state["path"] = get_path_to(controlled_node);
        controlled_state["enabled"] = _is_controlled_node_enabled(controlled_node);
        controlled_states.push_back(controlled_state);
    }
    state["controlled_nodes"] = controlled_states;

    checkpoint_runtime_state = state;

    for (int i = 0; i < checkpoint_nodes.size(); i++) {
        Node *checkpoint_node = Object::cast_to<Node>(checkpoint_nodes[i]);
        _set_checkpoint_visual(checkpoint_node, CHECKPOINT_DEFAULT_COLOR);
    }
    _set_checkpoint_visual(p_checkpoint_node, CHECKPOINT_ACTIVE_COLOR);
}

void MnmsLevelRunner::_restore_checkpoint_runtime_state() {
    if (checkpoint_runtime_state.is_empty()) {
        return;
    }

    collected_collectables = checkpoint_runtime_state.has("collected_collectables") ? (int)checkpoint_runtime_state["collected_collectables"] : 0;

    const Array collectable_states = checkpoint_runtime_state.has("collectables") ? (Array)checkpoint_runtime_state["collectables"] : Array();
    for (int i = 0; i < collectable_states.size(); i++) {
        const Dictionary collectable_state = collectable_states[i];
        if (!collectable_state.has("path")) {
            continue;
        }

        Node *collectable_node = Object::cast_to<Node>(get_node_or_null(NodePath(collectable_state["path"])));
        if (collectable_node == nullptr) {
            continue;
        }

        const bool collected = collectable_state.has("collected") && (bool)collectable_state["collected"];
        collectable_node->set_meta("collected", collected);

        Area2D *collectable_area = Object::cast_to<Area2D>(collectable_node);
        if (collectable_area != nullptr) {
            collectable_area->set_deferred("monitoring", !collected);
        }

        CanvasItem *visual = Object::cast_to<CanvasItem>(collectable_node->get_node_or_null("Visual"));
        if (visual != nullptr) {
            visual->set_visible(!collected);
        }
    }

    const Array fragile_states = checkpoint_runtime_state.has("fragile_nodes") ? (Array)checkpoint_runtime_state["fragile_nodes"] : Array();
    for (int i = 0; i < fragile_states.size(); i++) {
        const Dictionary fragile_state = fragile_states[i];
        if (!fragile_state.has("path")) {
            continue;
        }

        Node *fragile_node = Object::cast_to<Node>(get_node_or_null(NodePath(fragile_state["path"])));
        if (fragile_node == nullptr) {
            continue;
        }

        const bool broken = fragile_state.has("fragile_broken") && (bool)fragile_state["fragile_broken"];
        const int stage = fragile_state.has("fragile_stage") ? (int)fragile_state["fragile_stage"] : 0;
        const double progress = fragile_state.has("fragile_progress") ? (double)fragile_state["fragile_progress"] : 0.0;
        const int active_bodies = fragile_state.has("fragile_active_bodies") ? (int)fragile_state["fragile_active_bodies"] : 0;

        fragile_node->set_meta("fragile_broken", broken);
        fragile_node->set_meta("fragile_stage", stage);
        fragile_node->set_meta("fragile_progress", progress);
        fragile_node->set_meta("fragile_active_bodies", broken ? 0 : active_bodies);

        CanvasItem *visual = Object::cast_to<CanvasItem>(fragile_node->get_node_or_null("Visual"));
        Area2D *detector = Object::cast_to<Area2D>(fragile_node->get_node_or_null("Detector"));
        Object *collision_shape_object = fragile_node->get_node_or_null("CollisionShape2D");
        const int default_layer = fragile_node->has_meta("default_collision_layer") ? (int)fragile_node->get_meta("default_collision_layer") : 0;
        const int default_mask = fragile_node->has_meta("default_collision_mask") ? (int)fragile_node->get_meta("default_collision_mask") : 0;

        if (broken) {
            if (visual != nullptr) {
                visual->set_modulate(Color(1.0, 1.0, 1.0, 0.15));
            }
            if (detector != nullptr) {
                detector->set_deferred("monitoring", false);
            }
            if (collision_shape_object != nullptr) {
                collision_shape_object->set_deferred("disabled", true);
            }
            fragile_node->set_deferred("collision_layer", 0);
            fragile_node->set_deferred("collision_mask", 0);
        } else {
            if (visual != nullptr) {
                Color modulate = Color(1, 1, 1, 1);
                if (stage >= 2) {
                    modulate = Color(1.0, 0.72, 0.72, 1.0);
                } else if (stage >= 1) {
                    modulate = Color(1.0, 0.88, 0.78, 1.0);
                }
                visual->set_modulate(modulate);
            }
            if (detector != nullptr) {
                detector->set_deferred("monitoring", true);
            }
            if (collision_shape_object != nullptr) {
                collision_shape_object->set_deferred("disabled", false);
            }
            fragile_node->set_deferred("collision_layer", default_layer);
            fragile_node->set_deferred("collision_mask", default_mask);
        }
    }

    const Array moving_states = checkpoint_runtime_state.has("moving_nodes") ? (Array)checkpoint_runtime_state["moving_nodes"] : Array();
    for (int i = 0; i < moving_states.size(); i++) {
        const Dictionary moving_state = moving_states[i];
        if (!moving_state.has("path")) {
            continue;
        }

        Node *moving_node = Object::cast_to<Node>(get_node_or_null(NodePath(moving_state["path"])));
        Node2D *moving_node_2d = Object::cast_to<Node2D>(moving_node);
        if (moving_node == nullptr || moving_node_2d == nullptr) {
            continue;
        }

        moving_node->set_meta("moving_time", moving_state.has("moving_time") ? moving_state["moving_time"] : Variant(0.0));
        moving_node->set_meta("player_inside", moving_state.has("player_inside") && (bool)moving_state["player_inside"]);
        moving_node->set_meta("shadow_inside", moving_state.has("shadow_inside") && (bool)moving_state["shadow_inside"]);
        if (moving_state.has("position")) {
            moving_node_2d->set_position(moving_state["position"]);
        }
    }

    const Array swap_states = checkpoint_runtime_state.has("swap_nodes") ? (Array)checkpoint_runtime_state["swap_nodes"] : Array();
    for (int i = 0; i < swap_states.size(); i++) {
        const Dictionary swap_state = swap_states[i];
        if (!swap_state.has("path")) {
            continue;
        }

        Node *swap_node = Object::cast_to<Node>(get_node_or_null(NodePath(swap_state["path"])));
        if (swap_node == nullptr) {
            continue;
        }
        swap_node->set_meta("player_inside", swap_state.has("player_inside") && (bool)swap_state["player_inside"]);
        swap_node->set_meta("shadow_inside", swap_state.has("shadow_inside") && (bool)swap_state["shadow_inside"]);
    }

    const Array teleporter_states = checkpoint_runtime_state.has("teleporter_nodes") ? (Array)checkpoint_runtime_state["teleporter_nodes"] : Array();
    for (int i = 0; i < teleporter_states.size(); i++) {
        const Dictionary teleporter_state = teleporter_states[i];
        if (!teleporter_state.has("path")) {
            continue;
        }

        Node *teleporter_node = Object::cast_to<Node>(get_node_or_null(NodePath(teleporter_state["path"])));
        if (teleporter_node == nullptr) {
            continue;
        }
        teleporter_node->set_meta("player_inside", teleporter_state.has("player_inside") && (bool)teleporter_state["player_inside"]);
        teleporter_node->set_meta("shadow_inside", teleporter_state.has("shadow_inside") && (bool)teleporter_state["shadow_inside"]);
    }

    const Array switch_states = checkpoint_runtime_state.has("switch_nodes") ? (Array)checkpoint_runtime_state["switch_nodes"] : Array();
    for (int i = 0; i < switch_states.size(); i++) {
        const Dictionary switch_state = switch_states[i];
        if (!switch_state.has("path")) {
            continue;
        }

        Node *switch_node = Object::cast_to<Node>(get_node_or_null(NodePath(switch_state["path"])));
        if (switch_node == nullptr) {
            continue;
        }
        switch_node->set_meta("player_inside", switch_state.has("player_inside") && (bool)switch_state["player_inside"]);
        switch_node->set_meta("shadow_inside", switch_state.has("shadow_inside") && (bool)switch_state["shadow_inside"]);
    }

    const Array button_states = checkpoint_runtime_state.has("button_nodes") ? (Array)checkpoint_runtime_state["button_nodes"] : Array();
    for (int i = 0; i < button_states.size(); i++) {
        const Dictionary button_state = button_states[i];
        if (!button_state.has("path")) {
            continue;
        }

        Node *button_node = Object::cast_to<Node>(get_node_or_null(NodePath(button_state["path"])));
        if (button_node == nullptr) {
            continue;
        }

        const bool pressed = button_state.has("button_pressed") && (bool)button_state["button_pressed"];
        button_node->set_meta("active_bodies", button_state.has("active_bodies") ? button_state["active_bodies"] : Variant(0));
        button_node->set_meta("button_pressed", pressed);

        CanvasItem *visual = Object::cast_to<CanvasItem>(button_node->get_node_or_null("Visual"));
        if (visual != nullptr) {
            visual->set_modulate(pressed ? Color(0.95, 0.95, 0.95, 1.0) : Color(1, 1, 1, 1));
        }
    }

    const Array conveyor_states = checkpoint_runtime_state.has("conveyor_nodes") ? (Array)checkpoint_runtime_state["conveyor_nodes"] : Array();
    for (int i = 0; i < conveyor_states.size(); i++) {
        const Dictionary conveyor_state = conveyor_states[i];
        if (!conveyor_state.has("path")) {
            continue;
        }

        Node *conveyor_node = Object::cast_to<Node>(get_node_or_null(NodePath(conveyor_state["path"])));
        if (conveyor_node == nullptr) {
            continue;
        }
        conveyor_node->set_meta("player_inside", conveyor_state.has("player_inside") && (bool)conveyor_state["player_inside"]);
        conveyor_node->set_meta("shadow_inside", conveyor_state.has("shadow_inside") && (bool)conveyor_state["shadow_inside"]);
    }

    const Array pushable_states = checkpoint_runtime_state.has("pushable_nodes") ? (Array)checkpoint_runtime_state["pushable_nodes"] : Array();
    for (int i = 0; i < pushable_states.size(); i++) {
        const Dictionary pushable_state = pushable_states[i];
        if (!pushable_state.has("path")) {
            continue;
        }

        Node *pushable_node = Object::cast_to<Node>(get_node_or_null(NodePath(pushable_state["path"])));
        RigidBody2D *pushable_body = Object::cast_to<RigidBody2D>(pushable_node);
        if (pushable_node == nullptr || pushable_body == nullptr) {
            continue;
        }

        if (pushable_state.has("position")) {
            pushable_body->set_position(pushable_state["position"]);
        }
        pushable_body->set_linear_velocity(pushable_state.has("linear_velocity") ? Vector2(pushable_state["linear_velocity"]) : Vector2());
        pushable_body->set_angular_velocity(pushable_state.has("angular_velocity") ? (double)pushable_state["angular_velocity"] : 0.0);
        pushable_body->set_sleeping(false);
    }

    const Array controlled_states = checkpoint_runtime_state.has("controlled_nodes") ? (Array)checkpoint_runtime_state["controlled_nodes"] : Array();
    for (int i = 0; i < controlled_states.size(); i++) {
        const Dictionary controlled_state = controlled_states[i];
        if (!controlled_state.has("path")) {
            continue;
        }

        Node *controlled_node = Object::cast_to<Node>(get_node_or_null(NodePath(controlled_state["path"])));
        if (controlled_node == nullptr) {
            continue;
        }
        _set_controlled_node_enabled(controlled_node, controlled_state.has("enabled") && (bool)controlled_state["enabled"]);
    }

    for (int i = 0; i < exit_nodes.size(); i++) {
        Node *exit_node = Object::cast_to<Node>(exit_nodes[i]);
        _refresh_exit_open_state(exit_node);
    }

    for (int i = 0; i < checkpoint_nodes.size(); i++) {
        Node *checkpoint_node = Object::cast_to<Node>(checkpoint_nodes[i]);
        _set_checkpoint_visual(checkpoint_node, CHECKPOINT_DEFAULT_COLOR);
    }
    if (checkpoint_runtime_state.has("active_checkpoint_path")) {
        Node *active_checkpoint = Object::cast_to<Node>(get_node_or_null(NodePath(checkpoint_runtime_state["active_checkpoint_path"])));
        _set_checkpoint_visual(active_checkpoint, CHECKPOINT_ACTIVE_COLOR);
    }

    _set_notification_message(checkpoint_runtime_state.has("notification_message") ? String(checkpoint_runtime_state["notification_message"]) : String());
}

void MnmsLevelRunner::_set_checkpoint_visual(Node *p_checkpoint_node, const Color &p_color) {
    if (p_checkpoint_node == nullptr) {
        return;
    }

    CanvasItem *visual = Object::cast_to<CanvasItem>(p_checkpoint_node->get_node_or_null("Visual"));
    if (visual != nullptr) {
        visual->set_modulate(p_color);
    }
}

void MnmsLevelRunner::_set_exit_open(Node *p_exit_node, bool p_open) {
    if (p_exit_node == nullptr) {
        return;
    }

    Area2D *area = Object::cast_to<Area2D>(p_exit_node);
    if (area != nullptr) {
        area->set_deferred("monitoring", p_open);
    }

    CanvasItem *visual = Object::cast_to<CanvasItem>(p_exit_node->get_node_or_null("Visual"));
    if (visual != nullptr) {
        visual->set_modulate(p_open ? Color(1, 1, 1, 1) : Color(0.45, 0.45, 0.45, 0.9));
    }
}

void MnmsLevelRunner::_refresh_exit_open_state(Node *p_exit_node) {
    if (p_exit_node == nullptr) {
        return;
    }

    const bool collectables_ready = total_collectables <= 0 || collected_collectables >= total_collectables;
    const bool control_enabled = _is_controlled_node_enabled(p_exit_node);
    _set_exit_open(p_exit_node, collectables_ready && control_enabled);
}

String MnmsLevelRunner::_get_control_mode(Node *p_node) const {
    if (p_node == nullptr) {
        return "full";
    }
    if (p_node->has_meta("control_mode")) {
        return String(p_node->get_meta("control_mode"));
    }
    if (p_node->is_in_group("mnms_exit")) {
        return "exit";
    }
    if (p_node->is_in_group("mnms_moving_platform_root") ||
        p_node->is_in_group("mnms_moving_hazard_root") ||
        p_node->is_in_group("mnms_conveyor_root")) {
        return "motion";
    }
    return "full";
}

void MnmsLevelRunner::_set_controlled_node_enabled(Node *p_node, bool p_enabled) {
    if (p_node == nullptr) {
        return;
    }

    p_node->set_meta("enabled", p_enabled);

    const String control_mode = _get_control_mode(p_node);
    if (control_mode == "motion") {
        return;
    }
    if (control_mode == "exit") {
        _refresh_exit_open_state(p_node);
        return;
    }

    Variant default_layer_variant = p_node->has_meta("default_collision_layer") ? p_node->get_meta("default_collision_layer") : Variant(0);
    Variant default_mask_variant = p_node->has_meta("default_collision_mask") ? p_node->get_meta("default_collision_mask") : Variant(0);
    const int default_layer = (int)default_layer_variant;
    const int default_mask = (int)default_mask_variant;

    if (p_node->has_method("set_collision_layer")) {
        p_node->set_deferred("collision_layer", p_enabled ? default_layer : 0);
    }
    if (p_node->has_method("set_collision_mask")) {
        p_node->set_deferred("collision_mask", p_enabled ? default_mask : 0);
    }
    if (p_node->has_method("set_monitoring")) {
        p_node->set_deferred("monitoring", p_enabled);
    }

    Object *collision_shape_object = p_node->get_node_or_null("CollisionShape2D");
    if (collision_shape_object != nullptr) {
        collision_shape_object->set_deferred("disabled", !p_enabled);
    }

    CanvasItem *visual = Object::cast_to<CanvasItem>(p_node->get_node_or_null("Visual"));
    if (visual != nullptr) {
        visual->set_modulate(p_enabled ? Color(1, 1, 1, 1) : Color(0.4, 0.4, 0.4, 0.45));
    }

    Area2D *detector = Object::cast_to<Area2D>(p_node->get_node_or_null("Detector"));
    if (detector != nullptr) {
        detector->set_deferred("monitoring", p_enabled);
    }
}

bool MnmsLevelRunner::_is_controlled_node_enabled(Node *p_node) const {
    if (p_node == nullptr) {
        return false;
    }
    if (p_node->has_meta("enabled")) {
        return (bool)p_node->get_meta("enabled");
    }
    return true;
}

bool MnmsLevelRunner::_is_controlled_node_collidable(Node *p_node) const {
    if (p_node == nullptr) {
        return false;
    }

    if (!_is_controlled_node_enabled(p_node) && _get_control_mode(p_node) == "full") {
        return false;
    }
    return true;
}

void MnmsLevelRunner::_apply_control_signal(const String &p_id, const String &p_behaviour) {
    if (p_id.is_empty()) {
        return;
    }

    for (int i = 0; i < controlled_nodes.size(); i++) {
        Node *controlled_node = Object::cast_to<Node>(controlled_nodes[i]);
        if (controlled_node == nullptr || !controlled_node->has_meta("control_id")) {
            continue;
        }
        if (String(controlled_node->get_meta("control_id")) != p_id) {
            continue;
        }

        const bool current_enabled = _is_controlled_node_enabled(controlled_node);
        bool next_enabled = current_enabled;
        if (p_behaviour == "on") {
            next_enabled = true;
        } else if (p_behaviour == "off") {
            next_enabled = false;
        } else {
            next_enabled = !current_enabled;
        }
        _set_controlled_node_enabled(controlled_node, next_enabled);
    }
}

void MnmsLevelRunner::_update_fragile_blocks(double delta) {
    (void)delta;
    for (int i = 0; i < fragile_nodes.size(); i++) {
        Node *fragile_node = Object::cast_to<Node>(fragile_nodes[i]);
        if (fragile_node == nullptr || fragile_node->is_queued_for_deletion()) {
            continue;
        }
        if (fragile_node->has_meta("fragile_broken") && (bool)fragile_node->get_meta("fragile_broken")) {
            fragile_node->set_meta("fragile_active_bodies", 0);
            continue;
        }

        const bool shadow_only = fragile_node->has_meta("fragile_shadow_only") && (bool)fragile_node->get_meta("fragile_shadow_only");
        const int active_before = fragile_node->has_meta("fragile_active_bodies") ? (int)fragile_node->get_meta("fragile_active_bodies") : 0;
        const bool standing_now = shadow_only
            ? _is_body_standing_on_node(shadow, fragile_node)
            : _is_body_standing_on_node(player, fragile_node);

        fragile_node->set_meta("fragile_active_bodies", standing_now ? 1 : 0);
        if (standing_now && active_before <= 0) {
            _advance_fragile_state(fragile_node);
        }
    }
}

void MnmsLevelRunner::_update_button_states() {
    for (int i = 0; i < button_nodes.size(); i++) {
        Node *button_node = Object::cast_to<Node>(button_nodes[i]);
        if (button_node == nullptr || button_node->is_queued_for_deletion()) {
            continue;
        }

        const int active_bodies = _count_supported_bodies_on_node(button_node, true, true, true);
        button_node->set_meta("active_bodies", active_bodies);
        const bool pressed_now = active_bodies > 0;
        const bool pressed_before = button_node->has_meta("button_pressed") && (bool)button_node->get_meta("button_pressed");
        if (pressed_now == pressed_before) {
            continue;
        }

        button_node->set_meta("button_pressed", pressed_now);
        CanvasItem *visual = Object::cast_to<CanvasItem>(button_node->get_node_or_null("Visual"));
        if (visual != nullptr) {
            visual->set_modulate(pressed_now ? Color(0.95, 0.95, 0.95, 1.0) : Color(1, 1, 1, 1));
        }

        if (!button_node->has_meta("control_id")) {
            continue;
        }

        const String behaviour = button_node->has_meta("behaviour") ? String(button_node->get_meta("behaviour")) : String("toggle");
        const String control_id = String(button_node->get_meta("control_id"));
        if (pressed_now || behaviour == "toggle") {
            _apply_control_signal(control_id, behaviour);
        }
    }
}

Vector2 MnmsLevelRunner::_sample_moving_offset(Node *p_node, double p_time) const {
    if (p_node == nullptr || !p_node->has_meta("moving_points")) {
        return Vector2();
    }

    const Array points = p_node->get_meta("moving_points");
    Vector2 previous_point;
    double t = p_time;
    for (int i = 0; i < points.size(); i++) {
        Dictionary point = points[i];
        const Vector2 current_point(
            point.has("x") ? (double)point["x"] : 0.0,
            point.has("y") ? (double)point["y"] : 0.0
        );
        const double segment_time = point.has("t") ? (double)point["t"] : 0.0;
        if (segment_time <= 0.0) {
            previous_point = current_point;
            continue;
        }
        if (t >= 0.0 && t < segment_time) {
            const double weight = t / segment_time;
            return previous_point.lerp(current_point, weight);
        }
        if (i == points.size() - 1 && Math::is_equal_approx(t, segment_time)) {
            return current_point;
        }
        t -= segment_time;
        previous_point = current_point;
    }

    return previous_point;
}

bool MnmsLevelRunner::_build_node_rect(Node *p_node, Rect2 &r_rect) const {
    Node2D *node_2d = Object::cast_to<Node2D>(p_node);
    if (node_2d == nullptr) {
        return false;
    }

    if (p_node->is_in_group("mnms_pushable") || p_node->is_in_group("mnms_shadow_pushable")) {
        return _build_body_rect(node_2d, node_2d->get_position(), r_rect);
    }

    Vector2 tile_size = Vector2(50, 50);
    if (p_node->has_meta("tile_size")) {
        tile_size = Vector2(p_node->get_meta("tile_size"));
    }
    r_rect = Rect2(node_2d->get_position(), tile_size);
    return true;
}

bool MnmsLevelRunner::_body_uses_shadow_support(Node2D *p_body) const {
    if (p_body == nullptr) {
        return false;
    }
    return p_body->is_in_group("mnms_shadow") || p_body->is_in_group("mnms_shadow_pushable");
}

bool MnmsLevelRunner::_is_body_standing_on_node(Node2D *p_body, Node *p_node) const {
    if (p_body == nullptr || p_node == nullptr || p_node->is_queued_for_deletion()) {
        return false;
    }

    const bool body_uses_shadow_support = _body_uses_shadow_support(p_body);
    bool supports_body = false;
    if (p_node->is_in_group("mnms_button_root")) {
        supports_body = true;
    } else {
        supports_body = body_uses_shadow_support
            ? (p_node->is_in_group("mnms_solid") || p_node->is_in_group("mnms_shadow_solid"))
            : p_node->is_in_group("mnms_solid");
    }
    if (!supports_body) {
        return false;
    }

    Rect2 body_rect;
    Rect2 node_rect;
    if (!_build_body_rect(p_body, p_body->get_position(), body_rect) || !_build_node_rect(p_node, node_rect)) {
        return false;
    }

    const double overlap_left = MAX(body_rect.position.x, node_rect.position.x);
    const double overlap_right = MIN(body_rect.position.x + body_rect.size.x, node_rect.position.x + node_rect.size.x);
    if (overlap_right - overlap_left <= 2.0) {
        return false;
    }

    const double body_bottom = body_rect.position.y + body_rect.size.y;
    const double node_top = node_rect.position.y;
    if (body_rect.position.y >= node_top + 4.0) {
        return false;
    }
    return Math::abs(body_bottom - node_top) <= 8.0;
}

int32_t MnmsLevelRunner::_count_supported_bodies_on_node(Node *p_node, bool p_include_player, bool p_include_shadow, bool p_include_pushables) const {
    int32_t count = 0;
    if (p_include_player && _is_body_standing_on_node(player, p_node)) {
        count += 1;
    }
    if (p_include_shadow && _is_body_standing_on_node(shadow, p_node)) {
        count += 1;
    }
    if (p_include_pushables) {
        for (int i = 0; i < pushable_nodes.size(); i++) {
            RigidBody2D *pushable_body = Object::cast_to<RigidBody2D>(pushable_nodes[i]);
            if (_is_body_standing_on_node(pushable_body, p_node)) {
                count += 1;
            }
        }
    }
    return count;
}

void MnmsLevelRunner::_carry_body(Node *p_node, const String &p_meta_name, const Vector2 &p_motion) {
    if (p_node == nullptr || p_motion == Vector2()) {
        return;
    }

    if ((p_meta_name == "player_inside" && player != nullptr && p_node->has_meta(p_meta_name) && (bool)p_node->get_meta(p_meta_name))) {
        player->set_position(player->get_position() + p_motion);
    }
    if ((p_meta_name == "shadow_inside" && shadow != nullptr && p_node->has_meta(p_meta_name) && (bool)p_node->get_meta(p_meta_name))) {
        shadow->set_position(shadow->get_position() + p_motion);
    }
}

void MnmsLevelRunner::_carry_pushables_on_node(Node *p_node, const Vector2 &p_motion) {
    if (p_node == nullptr || p_motion == Vector2()) {
        return;
    }

    for (int i = 0; i < pushable_nodes.size(); i++) {
        RigidBody2D *pushable_body = Object::cast_to<RigidBody2D>(pushable_nodes[i]);
        if (pushable_body == nullptr || pushable_body->is_queued_for_deletion()) {
            continue;
        }
        if (!_is_body_standing_on_node(pushable_body, p_node)) {
            continue;
        }

        pushable_body->set_position(pushable_body->get_position() + p_motion);
        pushable_body->set_linear_velocity(Vector2());
    }
}

void MnmsLevelRunner::_update_moving_nodes(double delta) {
    const double frame_delta = delta * 60.0;
    for (int i = 0; i < moving_nodes.size(); i++) {
        Node *node = Object::cast_to<Node>(moving_nodes[i]);
        Node2D *node_2d = Object::cast_to<Node2D>(node);
        if (node == nullptr || node_2d == nullptr || node->is_queued_for_deletion()) {
            continue;
        }
        if (!node->has_meta("moving_points") || !node->has_meta("moving_total_time") || !node->has_meta("moving_base_position")) {
            continue;
        }

        const double total_time = (double)node->get_meta("moving_total_time");
        if (total_time <= 0.0) {
            continue;
        }

        const bool player_supported = _is_body_standing_on_node(player, node);
        const bool shadow_supported = _is_body_standing_on_node(shadow, node);
        node->set_meta("player_inside", player_supported);
        node->set_meta("shadow_inside", shadow_supported);

        const bool enabled = _is_controlled_node_enabled(node);
        const bool loops = !node->has_meta("moving_loops") || (bool)node->get_meta("moving_loops");
        double current_time = node->has_meta("moving_time") ? (double)node->get_meta("moving_time") : 0.0;
        const Vector2 old_offset = _sample_moving_offset(node, current_time);
        double next_time = current_time;
        bool auto_disable = false;
        if (enabled) {
            next_time += frame_delta;
            if (loops) {
                while (next_time >= total_time) {
                    next_time -= total_time;
                }
            } else if (next_time > total_time) {
                next_time = total_time;
            }

            if (!loops) {
                const Array points = node->get_meta("moving_points");
                double cursor = 0.0;
                for (int j = 0; j < points.size(); j++) {
                    const Dictionary point = points[j];
                    const double segment_time = point.has("t") ? (double)point["t"] : 0.0;
                    if (segment_time <= 0.0) {
                        if (current_time < cursor + 0.001 && next_time >= cursor - 0.001) {
                            next_time = cursor;
                            auto_disable = true;
                            break;
                        }
                        continue;
                    }
                    cursor += segment_time;
                }
            }
        }

        const Vector2 new_offset = _sample_moving_offset(node, next_time);
        const Vector2 motion = new_offset - old_offset;
        const Vector2 base_position = node->get_meta("moving_base_position");
        node_2d->set_position(base_position + new_offset);
        node->set_meta("moving_time", next_time);
        if (auto_disable) {
            _set_controlled_node_enabled(node, false);
        }

        if (enabled && node->is_in_group("mnms_moving_platform_root")) {
            _carry_body(node, "player_inside", motion);
            _carry_body(node, "shadow_inside", motion);
            _carry_pushables_on_node(node, motion);
        }
    }
}

void MnmsLevelRunner::_update_conveyors(double delta) {
    for (int i = 0; i < conveyor_nodes.size(); i++) {
        Node *node = Object::cast_to<Node>(conveyor_nodes[i]);
        if (node == nullptr || node->is_queued_for_deletion() || !_is_controlled_node_enabled(node)) {
            continue;
        }

        const bool player_supported = _is_body_standing_on_node(player, node);
        const bool shadow_supported = _is_body_standing_on_node(shadow, node);
        node->set_meta("player_inside", player_supported);
        node->set_meta("shadow_inside", shadow_supported);

        const double speed_px_per_frame = node->has_meta("conveyor_speed_px_per_frame") ? (double)node->get_meta("conveyor_speed_px_per_frame") : 0.0;
        if (Math::is_zero_approx(speed_px_per_frame)) {
            continue;
        }

        const Vector2 conveyor_motion(speed_px_per_frame * delta * 60.0, 0.0);
        _carry_body(node, "player_inside", conveyor_motion);
        _carry_body(node, "shadow_inside", conveyor_motion);
        _carry_pushables_on_node(node, conveyor_motion);
    }
}

void MnmsLevelRunner::_try_activate_checkpoints() {
    if (player == nullptr || shadow == nullptr) {
        return;
    }

    for (int i = 0; i < checkpoint_nodes.size(); i++) {
        Node *checkpoint_node = Object::cast_to<Node>(checkpoint_nodes[i]);
        if (checkpoint_node == nullptr || checkpoint_node->is_queued_for_deletion()) {
            continue;
        }

        const bool player_inside = checkpoint_node->has_meta("player_inside") && (bool)checkpoint_node->get_meta("player_inside");
        const bool shadow_inside = checkpoint_node->has_meta("shadow_inside") && (bool)checkpoint_node->get_meta("shadow_inside");
        const bool player_trigger = player_inside && _body_action_pressed(player);
        const bool shadow_trigger = shadow_inside && _body_action_pressed(shadow);
        if (!player_trigger && !shadow_trigger) {
            continue;
        }

        _activate_checkpoint(player->get_position(), shadow->get_position(), checkpoint_node);
        return;
    }
}

void MnmsLevelRunner::_try_activate_swap() {
    if (player == nullptr || shadow == nullptr) {
        return;
    }

    for (int i = 0; i < swap_nodes.size(); i++) {
        Node *swap_node = Object::cast_to<Node>(swap_nodes[i]);
        if (swap_node == nullptr || swap_node->is_queued_for_deletion()) {
            continue;
        }

        const bool player_inside = swap_node->has_meta("player_inside") && (bool)swap_node->get_meta("player_inside");
        const bool shadow_inside = swap_node->has_meta("shadow_inside") && (bool)swap_node->get_meta("shadow_inside");
        const bool player_trigger = player_inside && _body_action_pressed(player);
        const bool shadow_trigger = shadow_inside && _body_action_pressed(shadow);
        if (!player_trigger && !shadow_trigger) {
            continue;
        }

        const Vector2 old_player_position = player->get_position();
        const Vector2 old_shadow_position = shadow->get_position();
        if (!_can_body_fit_at(player, old_shadow_position, player, shadow) ||
            !_can_body_fit_at(shadow, old_player_position, player, shadow)) {
            UtilityFunctions::print("MNMS: swap refuse, destination bloquee.");
            return;
        }

        player->set_position(old_shadow_position);
        player->set_velocity(Vector2());
        shadow->set_position(old_player_position);
        shadow->set_velocity(Vector2());
        shadow->set_spawn_position(old_player_position);
        UtilityFunctions::print("MNMS: swap active.");
        emit_signal("swap_activated");
        return;
    }
}

void MnmsLevelRunner::_try_activate_switches() {
    for (int i = 0; i < switch_nodes.size(); i++) {
        Node *switch_node = Object::cast_to<Node>(switch_nodes[i]);
        if (switch_node == nullptr || switch_node->is_queued_for_deletion()) {
            continue;
        }

        const bool player_inside = switch_node->has_meta("player_inside") && (bool)switch_node->get_meta("player_inside");
        const bool shadow_inside = switch_node->has_meta("shadow_inside") && (bool)switch_node->get_meta("shadow_inside");
        const bool player_trigger = player_inside && _body_action_pressed(player);
        const bool shadow_trigger = shadow_inside && _body_action_pressed(shadow);
        if (!player_trigger && !shadow_trigger) {
            continue;
        }
        if (!switch_node->has_meta("control_id")) {
            continue;
        }

        const String control_id = String(switch_node->get_meta("control_id"));
        const String behaviour = switch_node->has_meta("behaviour") ? String(switch_node->get_meta("behaviour")) : String("toggle");
        _apply_control_signal(control_id, behaviour);
        UtilityFunctions::print("MNMS: switch active -> ", control_id);
        return;
    }
}

void MnmsLevelRunner::_try_activate_teleporters() {
    for (int i = 0; i < teleporter_nodes.size(); i++) {
        Node *teleporter_node = Object::cast_to<Node>(teleporter_nodes[i]);
        if (teleporter_node == nullptr || teleporter_node->is_queued_for_deletion()) {
            continue;
        }
        if (!_is_controlled_node_enabled(teleporter_node)) {
            continue;
        }

        const bool automatic = teleporter_node->has_meta("automatic") && (bool)teleporter_node->get_meta("automatic");

        if (player != nullptr && teleporter_node->has_meta("player_inside") && (bool)teleporter_node->get_meta("player_inside")) {
            if (automatic || _body_action_pressed(player)) {
                if (_teleport_body_via(player, teleporter_node)) {
                    return;
                }
            }
        }
        if (shadow != nullptr && teleporter_node->has_meta("shadow_inside") && (bool)teleporter_node->get_meta("shadow_inside")) {
            if (automatic || _body_action_pressed(shadow)) {
                if (_teleport_body_via(shadow, teleporter_node)) {
                    return;
                }
            }
        }
    }
}

bool MnmsLevelRunner::_body_action_pressed(Node2D *p_body) const {
    if (p_body == nullptr || !p_body->has_meta("mnms_action_pressed")) {
        return false;
    }
    return (bool)p_body->get_meta("mnms_action_pressed");
}

void MnmsLevelRunner::_set_notification_message(const String &p_message) {
    if (current_notification_message == p_message) {
        return;
    }
    current_notification_message = p_message;
    emit_signal("notification_changed", current_notification_message);
}

void MnmsLevelRunner::_emit_runtime_state_changed() {
    emit_signal("runtime_state_changed", elapsed_ticks, recordings_used, get_target_time(), get_target_recordings());
}

Node *MnmsLevelRunner::_find_teleporter_by_id(const String &p_id) const {
    for (int i = 0; i < teleporter_nodes.size(); i++) {
        Node *teleporter_node = Object::cast_to<Node>(teleporter_nodes[i]);
        if (teleporter_node == nullptr || !teleporter_node->has_meta("control_id")) {
            continue;
        }
        if (String(teleporter_node->get_meta("control_id")) == p_id) {
            return teleporter_node;
        }
    }
    return nullptr;
}

bool MnmsLevelRunner::_build_body_rect(Node2D *p_body, const Vector2 &p_position, Rect2 &r_rect) const {
    if (p_body == nullptr) {
        return false;
    }

    CollisionShape2D *collision_shape = Object::cast_to<CollisionShape2D>(p_body->get_node_or_null("CollisionShape2D"));
    if (collision_shape == nullptr) {
        return false;
    }

    Ref<RectangleShape2D> rectangle = collision_shape->get_shape();
    if (rectangle.is_null()) {
        return false;
    }

    const Vector2 size = rectangle->get_size();
    const Vector2 center = p_position + collision_shape->get_position();
    r_rect = Rect2(center - size * 0.5, size);
    return true;
}

bool MnmsLevelRunner::_node_blocks_body_at(Node *p_node, Node2D *p_body, const Rect2 &p_body_rect, Node *p_ignore_a, Node *p_ignore_b) const {
    if (p_node == nullptr || p_node == p_ignore_a || p_node == p_ignore_b || p_node->is_queued_for_deletion()) {
        return false;
    }
    if (!_is_controlled_node_collidable(p_node)) {
        return false;
    }

    const bool body_is_shadow = p_body->is_in_group("mnms_shadow");
    const bool blocks_body = body_is_shadow
        ? (p_node->is_in_group("mnms_solid") || p_node->is_in_group("mnms_shadow_solid"))
        : p_node->is_in_group("mnms_solid");
    if (!blocks_body) {
        return false;
    }

    Node2D *node_2d = Object::cast_to<Node2D>(p_node);
    if (node_2d == nullptr) {
        return false;
    }

    Vector2 tile_size = Vector2(50, 50);
    if (p_node->has_meta("tile_size")) {
        tile_size = Vector2(p_node->get_meta("tile_size"));
    }
    const Rect2 obstacle_rect(node_2d->get_position(), tile_size);
    return obstacle_rect.intersects(p_body_rect);
}

bool MnmsLevelRunner::_can_body_fit_at(Node2D *p_body, const Vector2 &p_position, Node *p_ignore_a, Node *p_ignore_b) const {
    Rect2 body_rect;
    if (!_build_body_rect(p_body, p_position, body_rect)) {
        return true;
    }

    if (body_rect.position.y < 0.0) {
        return false;
    }
    if (current_level.is_valid()) {
        const Vector2i world_size = current_level->get_world_size();
        if (body_rect.position.x < 0.0 || body_rect.position.x + body_rect.size.x > world_size.x) {
            return false;
        }
        if (body_rect.position.y + body_rect.size.y > world_size.y) {
            return false;
        }
    }

    if (tile_layer == nullptr) {
        return true;
    }

    Array nodes_to_check = tile_layer->get_children();
    for (int i = 0; i < nodes_to_check.size(); i++) {
        Node *node = Object::cast_to<Node>(nodes_to_check[i]);
        if (_node_blocks_body_at(node, p_body, body_rect, p_ignore_a, p_ignore_b)) {
            return false;
        }
    }

    return true;
}

void MnmsLevelRunner::_advance_fragile_state(Node *p_fragile_node) {
    if (p_fragile_node == nullptr || p_fragile_node->is_queued_for_deletion()) {
        return;
    }
    if (p_fragile_node->has_meta("fragile_broken") && (bool)p_fragile_node->get_meta("fragile_broken")) {
        return;
    }

    int stage = p_fragile_node->has_meta("fragile_stage") ? (int)p_fragile_node->get_meta("fragile_stage") : 0;
    stage += 1;
    p_fragile_node->set_meta("fragile_stage", stage);
    p_fragile_node->set_meta("fragile_progress", (double)stage);

    if (stage >= 3) {
        _break_fragile(p_fragile_node);
        return;
    }

    CanvasItem *visual = Object::cast_to<CanvasItem>(p_fragile_node->get_node_or_null("Visual"));
    if (visual == nullptr) {
        return;
    }

    if (stage >= 2) {
        visual->set_modulate(Color(1.0, 0.72, 0.72, 1.0));
    } else {
        visual->set_modulate(Color(1.0, 0.88, 0.78, 1.0));
    }
}

bool MnmsLevelRunner::_teleport_body_via(Node2D *p_body, Node *p_teleporter_node) {
    if (p_body == nullptr || p_teleporter_node == nullptr || !p_teleporter_node->has_meta("destination")) {
        return false;
    }

    const String current_teleporter_id = String::num_int64(p_teleporter_node->get_instance_id());
    if (p_body->has_meta("last_teleporter_id") && String(p_body->get_meta("last_teleporter_id")) == current_teleporter_id) {
        return false;
    }

    Node *destination_node = _find_teleporter_by_id(String(p_teleporter_node->get_meta("destination")));
    if (destination_node == nullptr || !_is_controlled_node_enabled(destination_node)) {
        return false;
    }

    const Vector2 destination_position = Object::cast_to<Node2D>(destination_node)->get_position();
    Vector2 destination_size = Vector2(50, 50);
    if (destination_node->has_meta("tile_size")) {
        destination_size = destination_node->get_meta("tile_size");
    }
    const Vector2 target_position = destination_position + Vector2(destination_size.x * 0.5, destination_size.y - 8.0);
    if (!_can_body_fit_at(p_body, target_position, p_teleporter_node, destination_node)) {
        UtilityFunctions::print("MNMS: teleport refuse, destination bloquee.");
        return false;
    }

    p_body->set_position(target_position);
    if (p_body->has_method("set_velocity")) {
        p_body->call("set_velocity", Vector2());
    }
    p_body->set_meta("last_teleporter_id", String::num_int64(destination_node->get_instance_id()));
    UtilityFunctions::print("MNMS: teleport -> ", String(p_teleporter_node->get_meta("destination")));
    return true;
}

void MnmsLevelRunner::_break_fragile(Node *p_fragile_node) {
    if (p_fragile_node == nullptr) {
        return;
    }

    p_fragile_node->set_meta("fragile_broken", true);
    p_fragile_node->set_meta("fragile_active_bodies", 0);

    CanvasItem *visual = Object::cast_to<CanvasItem>(p_fragile_node->get_node_or_null("Visual"));
    if (visual != nullptr) {
        visual->set_modulate(Color(1.0, 1.0, 1.0, 0.15));
    }

    Area2D *detector = Object::cast_to<Area2D>(p_fragile_node->get_node_or_null("Detector"));
    if (detector != nullptr) {
        detector->set_deferred("monitoring", false);
    }

    Object *collision_shape_object = p_fragile_node->get_node_or_null("CollisionShape2D");
    if (collision_shape_object != nullptr) {
        collision_shape_object->set_deferred("disabled", true);
    }
    p_fragile_node->set_deferred("collision_layer", 0);
    p_fragile_node->set_deferred("collision_mask", 0);
}

bool MnmsLevelRunner::_is_completion_body(Node2D *p_body) const {
    return p_body->is_in_group("mnms_player") || p_body->is_in_group("mnms_shadow");
}

void MnmsLevelRunner::_handle_shadow_failure() {
    if (shadow == nullptr || level_complete_pending) {
        return;
    }

    shadow->stop_replay();
    shadow->reset_to_spawn();
    shadow->set_velocity(Vector2());
    shadow->remove_meta("last_teleporter_id");
    _set_notification_message("Shadow perdu. Appuie sur Espace pour reenregistrer.");
}
