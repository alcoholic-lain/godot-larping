// src/mnms_player_shadow_system.h
#ifndef MNMS_PLAYER_SHADOW_SYSTEM_H
#define MNMS_PLAYER_SHADOW_SYSTEM_H

#include "mnms_player.h"
#include "mnms_replay_system.h"
#include "mnms_shadow.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/node_path.hpp>

namespace godot {

class MnmsPlayerShadowSystem : public Node {
    GDCLASS(MnmsPlayerShadowSystem, Node)

private:
    NodePath player_path;
    NodePath shadow_path;
    NodePath replay_system_path;
    bool shadow_view;
    bool recording_active;
    bool record_toggle_requested;
    bool record_cancel_requested;
    bool recording_started_this_tick;
    bool recording_cancelled_this_tick;
    bool replay_started_this_tick;
    Dictionary checkpoint_state;

    MnmsPlayer *_get_player() const;
    MnmsShadow *_get_shadow() const;
    MnmsReplaySystem *_get_replay_system() const;
    void _apply_camera_state();
    Dictionary _build_live_input_frame() const;
    void _apply_input_frame_meta(Node *p_target, const Dictionary &p_frame) const;
    void _clear_input_frame_meta(Node *p_target) const;

protected:
    static void _bind_methods();

public:
    MnmsPlayerShadowSystem();
    ~MnmsPlayerShadowSystem();

    void set_player_path(const NodePath &p_player_path);
    NodePath get_player_path() const;

    void set_shadow_path(const NodePath &p_shadow_path);
    NodePath get_shadow_path() const;

    void set_replay_system_path(const NodePath &p_replay_system_path);
    NodePath get_replay_system_path() const;

    void bind_runtime(const NodePath &p_player_path, const NodePath &p_shadow_path, const NodePath &p_replay_system_path);
    void reset_runtime_state();
    void request_record_toggle();
    void request_record_cancel();
    void capture_checkpoint_state();
    void restore_checkpoint_state();
    bool begin_recording();
    bool cancel_recording();
    bool end_recording_and_replay();
    bool restart_replay();
    bool stop_replay_playback();
    void stop_replay();
    void capture_frame();
    void toggle_shadow_view();
    bool is_shadow_view() const;
    bool is_recording() const;
    bool is_replaying() const;
    bool has_replay() const;
    bool consume_recording_started();
    bool consume_recording_cancelled();
    bool consume_replay_started();

    void _physics_process(double delta) override;
};

} // namespace godot

#endif
