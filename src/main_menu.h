#pragma once
#include <godot_cpp/classes/node2d.hpp>

namespace godot {

class MainMenu : public Node2D {
    GDCLASS(MainMenu, Node2D)

private:
    void start_game();
    void quit_game();

protected:
    static void _bind_methods();

public:
    void _ready() override;
    void _process(double delta) override;
};

} // namespace godot