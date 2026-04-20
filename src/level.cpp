#include "level.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/tween.hpp>

using namespace godot;







/*
let's remake color switch castle mode , you can mix colors in you color inventory  , and portals with diffrent colors kill you , we may add NPCs along the way 


*/

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