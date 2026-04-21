// src/mnms_player.h
#ifndef MNMS_PLAYER_H
#define MNMS_PLAYER_H

#include <godot_cpp/classes/character_body2d.hpp>
#include <godot_cpp/variant/array.hpp>

namespace godot {

class MnmsPlayer : public CharacterBody2D {
    GDCLASS(MnmsPlayer, CharacterBody2D)

private:
    double move_speed;
    double jump_velocity;
    double gravity;
    Dictionary character_states;
    Dictionary texture_cache;
    String current_state_name;
    bool facing_right;
    double animation_time;
    double last_input_axis;
    double grounded_visual_grace;
    double walk_visual_grace;
    bool visual_grounded;
    double visual_vertical_speed;
    double visual_horizontal_speed;
    int32_t animation_frame_index;
    bool supported_by_peer;
    Array recording_trail_points;
    bool recording_trail_enabled;

    void _update_visual_state(double delta);
    void _ensure_recording_trail_root();
    void _append_recording_trail_dot(const Vector2 &p_world_position);
    Dictionary _get_line_state_info() const;
    Node2D *_get_shadow_peer() const;
    bool _resolve_shadow_support();

protected:
    static void _bind_methods();

public:
    MnmsPlayer();
    ~MnmsPlayer();

    void set_character_states(const Dictionary &p_character_states);
    Dictionary get_character_states() const;
    void append_recording_trail_sample();
    void clear_recording_trail();
    void set_recording_trail_points(const Array &p_points);
    Array get_recording_trail_points() const;
    void set_recording_trail_enabled(bool p_enabled);
    bool is_recording_trail_enabled() const;
    void _ready() override;
    void _process(double delta) override;
    void _physics_process(double delta) override;
};

} // namespace godot

#endif
