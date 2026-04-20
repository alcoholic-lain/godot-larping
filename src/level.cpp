#include "level.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/tween.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>

using namespace godot;







/*
let's remake color switch castle mode , you can mix colors in you color inventory  , and portals with diffrent colors kill you , we may add NPCs along the way 


*/

void Level::_bind_methods() {}

void Level::_ready() {
    // Call Frame::_ready() first — creates the walls
    Frame::_ready();

    // Create UI layer for screen-relative elements
    ui_layer = memnew(CanvasLayer);
    ui_layer->set_name("CanvasLayer");
    add_child(ui_layer);

    // Create collected items label on UI layer
    collected_label = memnew(Label);
    collected_label->set_position(Vector2(10, 10));
    collected_label->set_text("Collected:\n");
    collected_label->set_name("CollectedLabel");
    ui_layer->add_child(collected_label);

    // Then fade in on top
    ColorRect *fade = memnew(ColorRect);
    fade->set_size(Vector2(get_level_width(), get_level_height()));
    fade->set_color(Color(0.0f, 0.0f, 0.0f, 1.0f));
    fade->set_name("FadeOverlay");
    add_child(fade);

    Ref<Tween> tween = create_tween();
    tween->tween_property(fade, "modulate:a", 0.0f, 1.0f);
}