#pragma once
#include "frame.h"
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>

namespace godot {

class Level : public Frame {
    GDCLASS(Level, Frame)

private:
    CanvasLayer* ui_layer = nullptr;
    Label* collected_label = nullptr;

protected:
    static void _bind_methods();

public:
    void _ready() override;
};

} // namespace godot