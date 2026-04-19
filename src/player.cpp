#include "player.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/input.hpp>

using namespace godot;

void Player::_bind_methods() {}

void Player::_physics_process(double delta) {
    Vector2 velocity = get_velocity();

    if (!is_on_floor())
        velocity.y += gravity * (float)delta;

    Input *input = Input::get_singleton();
    if (input->is_action_just_pressed("ui_accept") && is_on_floor())
        velocity.y = jump_velocity;

    float direction = input->get_axis("ui_left", "ui_right");
    velocity.x = direction * speed;

    set_velocity(velocity);
    move_and_slide();
}