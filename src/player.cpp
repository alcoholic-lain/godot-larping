#include "player.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <cmath>

using namespace godot;

// ─────────────────────────────────────────────
//  Build a filled circle as a Polygon2D
// ─────────────────────────────────────────────
static Polygon2D *make_circle_poly(float radius, Color color, int segs = 16) {
    Polygon2D *poly = memnew(Polygon2D);
    PackedVector2Array pts;
    for (int i = 0; i < segs; i++) {
        float angle = Math_TAU * (float)i / (float)segs;
        pts.push_back(Vector2(Math::cos(angle) * radius, Math::sin(angle) * radius));
    }
    poly->set_polygon(pts);
    poly->set_color(color);
    return poly;
}

void Player::_bind_methods() {}

// ─────────────────────────────────────────────
void Player::create_eyes() {
    const float SOCKET_R = 5.0f;
    const float PUPIL_R  = 2.5f;
    const Color WHITE    = Color(1, 1, 1, 1);
    const Color BLACK    = Color(0.05f, 0.05f, 0.05f, 1);

    const Vector2 LEFT_POS  = Vector2(-10, -14);
    const Vector2 RIGHT_POS = Vector2( 10, -14);

    // Left eye
    Node2D *l_socket = memnew(Node2D);
    l_socket->set_position(LEFT_POS);
    l_socket->add_child(make_circle_poly(SOCKET_R, WHITE));
    add_child(l_socket);

    left_pupil = memnew(Node2D);
    left_pupil->add_child(make_circle_poly(PUPIL_R, BLACK));
    l_socket->add_child(left_pupil);

    // Right eye
    Node2D *r_socket = memnew(Node2D);
    r_socket->set_position(RIGHT_POS);
    r_socket->add_child(make_circle_poly(SOCKET_R, WHITE));
    add_child(r_socket);

    right_pupil = memnew(Node2D);
    right_pupil->add_child(make_circle_poly(PUPIL_R, BLACK));
    r_socket->add_child(right_pupil);
}

// ─────────────────────────────────────────────
void Player::_ready() {
    // Build rounded-rectangle body matching CapsuleShape2D(radius=20, height=60)
    color_polygon = memnew(Polygon2D);

    PackedVector2Array points;
    const float W    = 20.0f;
    const float H    = 30.0f;
    const float R    = 10.0f;
    const int   SEGS = 10;

    struct Corner { float cx, cy, a0; };
    Corner corners[4] = {
        {  W - R,  H - R,  0.0f           },
        { -W + R,  H - R,  Math_PI * 0.5f },
        { -W + R, -H + R,  Math_PI        },
        {  W - R, -H + R,  Math_PI * 1.5f },
    };

    for (auto &c : corners) {
        for (int i = 0; i <= SEGS; i++) {
            float angle = c.a0 + (Math_PI * 0.5f) * (float)i / (float)SEGS;
            points.push_back(Vector2(
                c.cx + Math::cos(angle) * R,
                c.cy + Math::sin(angle) * R
            ));
        }
    }

    color_polygon->set_polygon(points);
    color_polygon->set_color(Color(1, 0.4f, 0.1f, 1));
    add_child(color_polygon);

    // Eyes drawn on top
    create_eyes();

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
        target     = Vector2(0, EYE_RADIUS);
        lerp_speed = LERP_FAST;
    } else if (!on_floor) {
        target     = Vector2(direction * EYE_RADIUS * 0.4f, -EYE_RADIUS);
        lerp_speed = LERP_FAST;
    } else if (direction != 0.0f) {
        target     = Vector2(direction * EYE_RADIUS, 0);
        lerp_speed = LERP_FAST;
    } else {
        lerp_speed = LERP_IDLE;

        idle_timer += (float)delta;
        if (idle_timer >= idle_interval) {
            idle_timer    = 0.0f;
            idle_interval = 1.2f + (float)UtilityFunctions::randf() * 2.0f;

            float angle = (float)UtilityFunctions::randf() * Math_TAU;
            float dist  = (float)UtilityFunctions::randf() * EYE_RADIUS;
            idle_target = Vector2(Math::cos(angle) * dist, Math::sin(angle) * dist);

            if (UtilityFunctions::randf() < 0.2f)
                idle_target.x = (direction >= 0 ? -1 : 1) * EYE_RADIUS * 0.8f;
        }
        target = idle_target;
    }

    float t = (float)delta * lerp_speed;
    left_offset  = left_offset.lerp(target, t);
    right_offset = right_offset.lerp(target, t);

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

    if (left_pupil && right_pupil)
        update_eyes(delta, direction, is_on_floor(), pressing_down);
}

// ─────────────────────────────────────────────
void Player::set_color(Color c) {
    if (color_polygon)
        color_polygon->set_color(c);
}

Color Player::get_color() const {
    if (color_polygon)
        return color_polygon->get_color();
    return Color(1, 0.4f, 0.1f, 1);
}
