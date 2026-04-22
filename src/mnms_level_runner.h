// src/mnms_level_runner.h
#ifndef MNMS_LEVEL_RUNNER_H
#define MNMS_LEVEL_RUNNER_H

#include "mnms_block_system.h"
#include "mnms_level_resource.h"
#include "mnms_player.h"
#include "mnms_shadow.h"
#include "mnms_theme_resource.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/classes/node2d.hpp>

namespace godot {

class Camera2D;

class MnmsLevelRunner : public Node2D {
    GDCLASS(MnmsLevelRunner, Node2D)

private:
    Ref<MnmsLevelResource> current_level;
    MnmsBlockSystem *block_system;
    Node2D *tile_layer;
    MnmsPlayer *player;
    MnmsShadow *shadow;
    Ref<MnmsThemeResource> theme;
    bool checkpoint_active;
    bool level_complete_pending;
    bool level_failed_pending;
    Vector2 player_spawn;
    Vector2 shadow_spawn;
    Vector2 checkpoint_player_position;
    Vector2 checkpoint_shadow_position;
    int32_t elapsed_ticks;
    int32_t recordings_used;
    int32_t checkpoint_elapsed_ticks;
    int32_t checkpoint_recordings_used;
    int32_t total_collectables;
    int32_t collected_collectables;
    Array checkpoint_nodes;
    Array collectable_nodes;
    Array exit_nodes;
    Array fragile_nodes;
    Array swap_nodes;
    Array teleporter_nodes;
    Array switch_nodes;
    Array button_nodes;
    Array moving_nodes;
    Array conveyor_nodes;
    Array pushable_nodes;
    Array controlled_nodes;
    String current_notification_message;
    Dictionary checkpoint_runtime_state;

    void _clear_level();
    void _spawn_player_shadow();
    void _configure_camera(Camera2D *p_camera) const;
    void _activate_checkpoint(const Vector2 &p_player_position, const Vector2 &p_shadow_position, Node *p_checkpoint_node);
    void _capture_checkpoint_runtime_state(Node *p_checkpoint_node);
    void _restore_checkpoint_runtime_state();
    void _set_checkpoint_visual(Node *p_checkpoint_node, const Color &p_color);
    void _set_exit_open(Node *p_exit_node, bool p_open);
    void _refresh_exit_open_state(Node *p_exit_node);
    String _get_control_mode(Node *p_node) const;
    void _set_controlled_node_enabled(Node *p_node, bool p_enabled);
    bool _is_controlled_node_enabled(Node *p_node) const;
    bool _is_controlled_node_collidable(Node *p_node) const;
    void _apply_control_signal(const String &p_id, const String &p_behaviour);
    void _update_fragile_blocks(double delta);
    void _update_button_states();
    void _update_moving_nodes(double delta);
    void _update_conveyors(double delta);
    void _try_activate_checkpoints();
    void _try_activate_swap();
    void _try_activate_switches();
    void _try_activate_teleporters();
    Vector2 _sample_moving_offset(Node *p_node, double p_time) const;
    bool _build_node_rect(Node *p_node, Rect2 &r_rect) const;
    bool _body_uses_shadow_support(Node2D *p_body) const;
    bool _is_body_standing_on_node(Node2D *p_body, Node *p_node) const;
    int32_t _count_supported_bodies_on_node(Node *p_node, bool p_include_player, bool p_include_shadow, bool p_include_pushables) const;
    void _carry_body(Node *p_node, const String &p_meta_name, const Vector2 &p_motion);
    void _carry_pushables_on_node(Node *p_node, const Vector2 &p_motion);
    Node *_find_teleporter_by_id(const String &p_id) const;
    bool _build_body_rect(Node2D *p_body, const Vector2 &p_position, Rect2 &r_rect) const;
    bool _node_blocks_body_at(Node *p_node, Node2D *p_body, const Rect2 &p_body_rect, Node *p_ignore_a = nullptr, Node *p_ignore_b = nullptr) const;
    bool _can_body_fit_at(Node2D *p_body, const Vector2 &p_position, Node *p_ignore_a = nullptr, Node *p_ignore_b = nullptr) const;
    void _advance_fragile_state(Node *p_fragile_node);
    bool _teleport_body_via(Node2D *p_body, Node *p_teleporter_node);
    void _break_fragile(Node *p_fragile_node);
    void _handle_shadow_failure();
    bool _is_completion_body(Node2D *p_body) const;
    bool _body_action_pressed(Node2D *p_body) const;
    void _set_notification_message(const String &p_message);
    void _emit_runtime_state_changed();

protected:
    static void _bind_methods();

public:
    MnmsLevelRunner();
    ~MnmsLevelRunner();

    void _ready() override;
    void _physics_process(double delta) override;
    void set_theme(const Ref<MnmsThemeResource> &p_theme);
    Ref<MnmsThemeResource> get_theme() const;
    void load_level(const Ref<MnmsLevelResource> &p_level);
    Ref<MnmsLevelResource> get_current_level() const;
    void restart_level();
    void load_checkpoint();
    bool has_checkpoint() const;
    MnmsPlayer *get_player() const;
    MnmsShadow *get_shadow() const;
    Vector2 get_player_spawn() const;
    Vector2 get_shadow_spawn() const;
    void increment_recordings_used();
    void set_recordings_used(int32_t p_recordings_used);
    int32_t get_recordings_used() const;
    int32_t get_elapsed_ticks() const;
    int32_t get_target_time() const;
    int32_t get_target_recordings() const;
    String get_current_notification_message() const;

    void _on_spikes_body_entered(Node2D *p_body);
    void _on_shadow_spikes_body_entered(Node2D *p_body);
    void _on_exit_body_entered(Node2D *p_body);
    void _on_checkpoint_body_entered(Node2D *p_body, Node *p_checkpoint_node);
    void _on_checkpoint_body_exited(Node2D *p_body, Node *p_checkpoint_node);
    void _on_collectable_body_entered(Node2D *p_body, Node *p_collectable_node);
    void _on_fragile_body_entered(Node2D *p_body, Node *p_fragile_node);
    void _on_fragile_body_exited(Node2D *p_body, Node *p_fragile_node);
    void _on_swap_body_entered(Node2D *p_body, Node *p_swap_node);
    void _on_swap_body_exited(Node2D *p_body, Node *p_swap_node);
    void _on_teleporter_body_entered(Node2D *p_body, Node *p_teleporter_node);
    void _on_teleporter_body_exited(Node2D *p_body, Node *p_teleporter_node);
    void _on_switch_body_entered(Node2D *p_body, Node *p_switch_node);
    void _on_switch_body_exited(Node2D *p_body, Node *p_switch_node);
    void _on_button_body_entered(Node2D *p_body, Node *p_button_node);
    void _on_button_body_exited(Node2D *p_body, Node *p_button_node);
    void _on_platform_body_entered(Node2D *p_body, Node *p_platform_node);
    void _on_platform_body_exited(Node2D *p_body, Node *p_platform_node);
    void _on_conveyor_body_entered(Node2D *p_body, Node *p_conveyor_node);
    void _on_conveyor_body_exited(Node2D *p_body, Node *p_conveyor_node);
    void _on_notification_body_entered(Node2D *p_body, Node *p_notification_node);
    void _on_notification_body_exited(Node2D *p_body, Node *p_notification_node);
};

} // namespace godot

#endif
