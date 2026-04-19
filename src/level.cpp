#include "level.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/tween.hpp>

using namespace godot;

void Level::_bind_methods() {}

void Level::_ready() {
    // Call Frame::_ready() first — creates the walls
    Frame::_ready();

    // Then fade in on top
    ColorRect *fade = memnew(ColorRect);
    fade->set_size(Vector2(1152, 648));
    fade->set_color(Color(0.0f, 0.0f, 0.0f, 1.0f));
    fade->set_name("FadeOverlay");
    add_child(fade);

    Ref<Tween> tween = create_tween();
    tween->tween_property(fade, "modulate:a", 0.0f, 1.0f);
}