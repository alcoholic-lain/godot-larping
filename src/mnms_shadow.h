// src/mnms_shadow.h
#ifndef MNMS_SHADOW_H
#define MNMS_SHADOW_H

#include <godot_cpp/classes/character_body2d.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {

class MnmsShadow : public CharacterBody2D {
    GDCLASS(MnmsShadow, CharacterBody2D)

private:
    Array replay_frames;
    Vector2 spawn_position;
    double move_speed;
    double jump_velocity;
    double gravity;
    Dictionary character_states;
    Dictionary texture_cache;
    String current_state_name;
    bool facing_right;
    double animation_time;
    double last_axis;
    double grounded_visual_grace;
    double walk_visual_grace;
    bool visual_grounded;
    double visual_vertical_speed;
    double visual_horizontal_speed;
    int32_t replay_frame_index;
    int32_t animation_frame_index;
    bool replaying;
    bool supported_by_peer;

    void _update_visual_state(double delta);
    Node2D *_get_player_peer() const;
    bool _resolve_player_support();

protected:
    static void _bind_methods();

public:
    MnmsShadow();
    ~MnmsShadow();

    void _ready() override;
    void _process(double delta) override;
    void _physics_process(double delta) override;

    void set_character_states(const Dictionary &p_character_states);
    Dictionary get_character_states() const;
    void set_spawn_position(const Vector2 &p_spawn_position);
    Vector2 get_spawn_position() const;
    void reset_to_spawn();
    void start_replay(const Array &p_replay_frames);
    void stop_replay();
    bool is_replaying() const;
    Array get_replay_frames() const;
    int32_t get_replay_frame_index() const;
    void restore_replay_state(const Array &p_replay_frames, int32_t p_replay_frame_index, const Vector2 &p_position, const Vector2 &p_velocity);
    Dictionary peek_next_frame() const;
};

} // namespace godot

#endif
