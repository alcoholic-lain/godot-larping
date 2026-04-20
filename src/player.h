#pragma once
#include <godot_cpp/classes/character_body2d.hpp>
#include <godot_cpp/classes/node2d.hpp>

namespace godot {

class Player : public CharacterBody2D {
    GDCLASS(Player, CharacterBody2D)

private:
    float speed         = 200.0f;
    float jump_velocity = -600.0f;
    float gravity       = 980.0f;

    // ── eye nodes (pupils inside each eye socket) ──
    Node2D *left_pupil  = nullptr;
    Node2D *right_pupil = nullptr;

    // idle goof state
    float idle_timer     = 0.0f;   // counts up; triggers a new wander target
    float idle_interval  = 2.0f;   // seconds between random flicks
    Vector2 idle_target  = Vector2(0, 0);  // current wander target (local to eye socket)

    // smooth lerp state
    Vector2 left_offset  = Vector2(0, 0);
    Vector2 right_offset = Vector2(0, 0);

    static constexpr float EYE_RADIUS   = 3.5f;  // max pupil travel from socket centre
    static constexpr float LERP_FAST    = 12.0f; // movement mode
    static constexpr float LERP_IDLE    = 5.0f;  // idle drift

    void create_eyes();
    void update_eyes(double delta, float direction, bool on_floor, bool jumping_down);

protected:
    static void _bind_methods();

public:
    void _ready() override;
    void _physics_process(double delta) override;
};

} // namespace godot
