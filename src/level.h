#pragma once
#include "frame.h"

namespace godot {

class Level : public Frame {
    GDCLASS(Level, Frame)

protected:
    static void _bind_methods();

public:
    void _ready() override;
};

} // namespace godot