#pragma once
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/input.hpp>

namespace godot {

class WinScreen : public Node2D {
    GDCLASS(WinScreen, Node2D)

protected:
    static void _bind_methods();

public:
    void _ready() override;
    void _process(double delta) override;
};

} // namespace godot