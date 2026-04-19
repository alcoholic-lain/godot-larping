#include "main_menu.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/tween.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void MainMenu::_bind_methods() {
    ClassDB::bind_method(D_METHOD("start_game"), &MainMenu::start_game);
}

void MainMenu::_ready() {
    // Background
    ColorRect *bg = memnew(ColorRect);
    bg->set_size(Vector2(1152, 648));
    bg->set_color(Color(0.04f, 0.04f, 0.12f, 1.0f));
    add_child(bg);

    // Title
    Label *title = memnew(Label);
    title->set_text("[ Game Title ]");
    title->set_position(Vector2(276, 150));
    title->add_theme_font_size_override("font_size", 52);
    title->add_theme_color_override("font_color", Color(1.0f, 0.85f, 0.0f, 1.0f));
    add_child(title);

    // Subtitle
    Label *subtitle = memnew(Label);
    subtitle->set_text("[ Game description placeholder ]");
    subtitle->set_position(Vector2(276, 230));
    subtitle->add_theme_font_size_override("font_size", 22);
    subtitle->add_theme_color_override("font_color", Color(0.6f, 0.6f, 0.8f, 1.0f));
    add_child(subtitle);

    // Divider
    ColorRect *divider = memnew(ColorRect);
    divider->set_size(Vector2(600, 2));
    divider->set_position(Vector2(276, 280));
    divider->set_color(Color(1.0f, 0.85f, 0.0f, 0.4f));
    add_child(divider);

    // Start prompt
    Label *start = memnew(Label);
    start->set_name("StartLabel");
    start->set_text("Press ENTER to begin");
    start->set_position(Vector2(376, 340));
    start->add_theme_font_size_override("font_size", 32);
    start->add_theme_color_override("font_color", Color(1.0f, 1.0f, 1.0f, 1.0f));
    add_child(start);

    // Quit hint
    Label *quit = memnew(Label);
    quit->set_text("ESC to quit");
    quit->set_position(Vector2(476, 400));
    quit->add_theme_font_size_override("font_size", 20);
    quit->add_theme_color_override("font_color", Color(0.5f, 0.5f, 0.5f, 1.0f));
    add_child(quit);

    // Fade in overlay
    ColorRect *fade = memnew(ColorRect);
    fade->set_size(Vector2(1152, 648));
    fade->set_color(Color(0.0f, 0.0f, 0.0f, 1.0f));
    fade->set_name("FadeOverlay");
    add_child(fade);

    Ref<Tween> tween = create_tween();
    tween->tween_property(fade, "modulate:a", 0.0f, 1.5f);

    // Pulse the start label
    Ref<Tween> pulse = create_tween();
    pulse->set_loops();
    pulse->tween_property(start, "modulate:a", 0.2f, 0.8f);
    pulse->tween_property(start, "modulate:a", 1.0f, 0.8f);
}

void MainMenu::_process(double delta) {
    Input *input = Input::get_singleton();

    if (input->is_action_just_pressed("ui_accept")) {
        start_game();
    }
    if (input->is_action_just_pressed("ui_cancel")) {
        quit_game();
    }
}

void MainMenu::start_game() {
    ColorRect *fade = get_node<ColorRect>("FadeOverlay");
    if (fade) {
        Ref<Tween> tween = create_tween();
        tween->tween_property(fade, "modulate:a", 1.0f, 0.8f);
        tween->tween_callback(Callable(get_tree(), "change_scene_to_file")
            .bind("res://level1.tscn"));
    } else {
        get_tree()->change_scene_to_file("res://level1.tscn");
    }
}

void MainMenu::quit_game() {
    get_tree()->quit();
}