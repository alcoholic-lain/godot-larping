#include "player.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <cmath>

using namespace godot;

// ─────────────────────────────────────────────
//  tiny helper: draw a filled circle via a
//  square ColorRect rotated 45° — cheap & clean
// ─────────────────────────────────────────────
static Node2D *make_circle(float radius, Color color) {
    // We use a ColorRect sized to the diameter and offset by -radius so
    // its visual centre sits at (0,0) of the returned Node2D.
    Node2D *pivot = memnew(Node2D);

    ColorRect *rect = memnew(ColorRect);
    float d = radius * 2.0f;
    rect->set_size(Vector2(d, d));
    rect->set_position(Vector2(-radius, -radius));
    rect->set_color(color);
    pivot->add_child(rect);

    return pivot;
}

void Player::_bind_methods() {}

// ─────────────────────────────────────────────
void Player::create_eyes() {
    // Eye sockets: white discs
    // Player body is assumed ~32×32; eyes sit at roughly y = -8 from centre
    const float SOCKET_R = 6.0f;
    const float PUPIL_R  = 3.0f;
    const Color WHITE     = Color(1, 1, 1, 1);
    const Color BLACK     = Color(0.05f, 0.05f, 0.05f, 1);

    // offsets from player origin (centre of body)
    const Vector2 LEFT_POS  = Vector2(-9, -8);
    const Vector2 RIGHT_POS = Vector2( 9, -8);

    // left socket
    Node2D *l_socket = make_circle(SOCKET_R, WHITE);
    l_socket->set_position(LEFT_POS);
    add_child(l_socket);

    // left pupil (child of socket so it inherits position)
    left_pupil = make_circle(PUPIL_R, BLACK);
    left_pupil->set_position(Vector2(0, 0));
    l_socket->add_child(left_pupil);

    // right socket
    Node2D *r_socket = make_circle(SOCKET_R, WHITE);
    r_socket->set_position(RIGHT_POS);
    add_child(r_socket);

    // right pupil
    right_pupil = make_circle(PUPIL_R, BLACK);
    right_pupil->set_position(Vector2(0, 0));
    r_socket->add_child(right_pupil);
}

// ─────────────────────────────────────────────
void Player::_ready() {
    create_eyes();
    // seed a random idle target
    idle_target = Vector2(
        (float)(UtilityFunctions::randf() * 2.0 - 1.0) * EYE_RADIUS,
        (float)(UtilityFunctions::randf() * 2.0 - 1.0) * EYE_RADIUS
    );
}

// ─────────────────────────────────────────────
void Player::update_eyes(double delta, float direction, bool on_floor, bool pressing_down) {
    Vector2 target;
    float   lerp_speed;

    if (pressing_down) {
        // ── look down ──
        target     = Vector2(0, EYE_RADIUS);
        lerp_speed = LERP_FAST;
    } else if (!on_floor) {
        // ── in the air: look up-ish ──
        target     = Vector2(direction * EYE_RADIUS * 0.4f, -EYE_RADIUS);
        lerp_speed = LERP_FAST;
    } else if (direction != 0.0f) {
        // ── moving: look sideways ──
        target     = Vector2(direction * EYE_RADIUS, 0);
        lerp_speed = LERP_FAST;
    } else {
        // ── idle: goofing around ──
        lerp_speed = LERP_IDLE;

        idle_timer += (float)delta;
        if (idle_timer >= idle_interval) {
            idle_timer    = 0.0f;
            idle_interval = 1.2f + (float)UtilityFunctions::randf() * 2.0f;

            // pick a new random target within the socket
            float angle = (float)UtilityFunctions::randf() * Math_TAU;
            float dist  = (float)UtilityFunctions::randf() * EYE_RADIUS;
            idle_target = Vector2(Math::cos(angle) * dist, Math::sin(angle) * dist);

            // 20% chance: do a quick cross-eye gag (pupils drift inward)
            if (UtilityFunctions::randf() < 0.2f) {
                idle_target.x = (direction >= 0 ? -1 : 1) * EYE_RADIUS * 0.8f;
            }
        }
        target = idle_target;
    }

    // smooth lerp both pupils toward target
    float t = (float)delta * lerp_speed;
    left_offset  = left_offset.lerp(target, t);
    right_offset = right_offset.lerp(target, t);

    // slight asymmetry: right eye lags a tiny bit for personality
    Vector2 right_lazy = right_offset.lerp(left_offset, 0.15f);

    left_pupil->set_position(left_offset);
    right_pupil->set_position(right_lazy);
}

// ─────────────────────────────────────────────
void Player::_physics_process(double delta) {
    Vector2 velocity = get_velocity();

    if (!is_on_floor())
        velocity.y += gravity * (float)delta;

    Input *input = Input::get_singleton();

    bool pressing_down = input->is_action_pressed("ui_down");

    if (input->is_action_just_pressed("ui_accept") && is_on_floor())
        velocity.y = jump_velocity;

    float direction = input->get_axis("ui_left", "ui_right");
    velocity.x = direction * speed;

    set_velocity(velocity);
    move_and_slide();

    // update eyes after movement is resolved
    if (left_pupil && right_pupil)
        update_eyes(delta, direction, is_on_floor(), pressing_down);
}
