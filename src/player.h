#pragma once
#include <godot_cpp/classes/character_body2d.hpp>

namespace godot {

class Player : public CharacterBody2D {
    GDCLASS(Player, CharacterBody2D)

private:
    float speed = 200.0f;
    float jump_velocity = -600.0f;
    float gravity = 980.0f;

protected:
    static void _bind_methods();

public:
    void _physics_process(double delta) override;
};

} // namespace godot