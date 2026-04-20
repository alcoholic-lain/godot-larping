#include "lose_screen.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/tween.hpp>

using namespace godot;

void LoseScreen::_bind_methods() {}

void LoseScreen::_ready() {
    // Dark background
    ColorRect *bg = memnew(ColorRect);
    bg->set_size(Vector2(1600, 900));
    bg->set_color(Color(0.1f, 0.05f, 0.05f, 1.0f));
    add_child(bg);

    // Red "YOU LOSE!" label
    Label *title = memnew(Label);
    title->set_text("YOU LOSE!");
    title->set_position(Vector2(376, 200));
    title->add_theme_font_size_override("font_size", 72);
    title->add_theme_color_override("font_color", Color(1.0f, 0.0f, 0.0f, 1.0f));
    add_child(title);

    // Subtitle
    Label *subtitle = memnew(Label);
    subtitle->set_text("Press ENTER to try again\nPress ESC to quit");
    subtitle->set_position(Vector2(376, 340));
    subtitle->add_theme_font_size_override("font_size", 28);
    subtitle->add_theme_color_override("font_color", Color(0.8f, 0.8f, 0.8f, 1.0f));
    add_child(subtitle);

    // Black overlay that fades out — creates fade-in effect
    ColorRect *fade = memnew(ColorRect);
    fade->set_size(Vector2(1600, 900));
    fade->set_color(Color(0.0f, 0.0f, 0.0f, 1.0f)); // start fully black
    fade->set_name("FadeOverlay");
    add_child(fade);

    // Tween the overlay from opaque to transparent
    Ref<Tween> tween = create_tween();
    tween->tween_property(fade, "modulate:a", 0.0f, 1.2f);
}

void LoseScreen::_process(double delta) {
    Input *input = Input::get_singleton();

    if (input->is_action_just_pressed("ui_accept")) {
        // Fade out then switch
        ColorRect *fade = get_node<ColorRect>("FadeOverlay");
        if (fade) {
            Ref<Tween> tween = create_tween();
            tween->tween_property(fade, "modulate:a", 1.0f, 0.6f);
            tween->tween_callback(Callable(get_tree(), "change_scene_to_file")
                .bind("res://level1.tscn"));
        } else {
            get_tree()->change_scene_to_file("res://level1.tscn");
        }
    }
    if (input->is_action_just_pressed("ui_cancel")) {
        get_tree()->quit();
    }
}