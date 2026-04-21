// src/mnms_game_director.cpp
#include "mnms_game_director.h"

#include "mnms_pack_resource.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/font_file.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/atlas_texture.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace {

struct MnmsActionBindingDef {
    const char *action;
    const char *label;
    Key default_key;
    bool configurable;
};

struct MnmsAchievementDef {
    const char *id;
    const char *title;
    const char *description;
    const char *stat_key;
    int32_t threshold;
};

static const MnmsActionBindingDef ACTION_BINDINGS[] = {
    {"ui_left", "Gauche", KEY_LEFT, true},
    {"ui_right", "Droite", KEY_RIGHT, true},
    {"ui_up", "Saut", KEY_UP, true},
    {"ui_down", "Action", KEY_DOWN, true},
    {"mnms_restart", "Recommencer", KEY_R, true},
    {"mnms_checkpoint", "Checkpoint / reprise", KEY_L, true},
    {"mnms_record_toggle", "Enregistrer / rejouer", KEY_SPACE, true},
    {"mnms_cancel_or_stop", "Annuler / stop replay", KEY_BACKSPACE, true},
    {"mnms_shadow_view", "Vue shadow", KEY_TAB, true},
    {"mnms_save_replay", "Sauvegarder replay", KEY_F5, true},
    {"mnms_replay_restart", "Relancer replay", KEY_F8, true},
    {"mnms_pause", "Pause", KEY_ESCAPE, true},
    {"mnms_level_select", "Choisir un niveau", KEY_F1, true},
    {"mnms_validation_prev", "Golden precedent", KEY_F6, false},
    {"mnms_validation_next", "Golden suivant", KEY_F7, false},
};

static const int32_t ACTION_BINDING_COUNT = sizeof(ACTION_BINDINGS) / sizeof(ACTION_BINDINGS[0]);

static const MnmsAchievementDef ACHIEVEMENTS[] = {
    {"newbie", "Newbie", "Terminer un niveau.", "levels_completed", 1},
    {"goodjob", "Good job!", "Obtenir une medaille or.", "gold_medals", 1},
    {"experienced", "Experienced player", "Terminer 50 niveaux.", "levels_completed", 50},
    {"expert", "Expert", "Obtenir 50 medailles or.", "gold_medals", 50},
    {"record100", "Recorder", "Lancer 100 enregistrements.", "recordings_started", 100},
    {"collect100", "Enriched", "Ramasser 100 collectables.", "collectables", 100},
    {"swap100", "Swapper", "Utiliser le swap 100 fois.", "swaps", 100},
    {"die1", "Be careful!", "Mourir une fois.", "deaths", 1},
    {"die50", "It doesn't matter...", "Mourir 50 fois.", "deaths", 50},
    {"save100", "Archiviste", "Sauvegarder 100 replays.", "replay_saves", 100},
    {"load100", "Analyste", "Charger 100 replays.", "replay_loads", 100},
    {"checkpoint100", "Routine de securite", "Activer 100 checkpoints.", "checkpoint_activations", 100},
};

static const int32_t ACHIEVEMENT_COUNT = sizeof(ACHIEVEMENTS) / sizeof(ACHIEVEMENTS[0]);
static const int32_t MAX_CLASSIC_LEVEL_COUNT = 9;

static String format_ticks_as_time(int32_t p_ticks) {
    if (p_ticks < 0) {
        return "-";
    }

    const int32_t total_seconds = p_ticks / 60;
    const int32_t minutes = total_seconds / 60;
    const int32_t seconds = total_seconds % 60;
    String out = String::num_int64(minutes);
    out += ":";
    if (seconds < 10) {
        out += "0";
    }
    out += String::num_int64(seconds);
    return out;
}

static Ref<Texture2D> load_texture_if_exists(const String &p_path) {
    return ResourceLoader::get_singleton()->load(p_path);
}

static Ref<FontFile> load_font_if_exists(const String &p_path) {
    return ResourceLoader::get_singleton()->load(p_path);
}

static Ref<StyleBoxFlat> make_flat_stylebox(
        const Color &p_background,
        const Color &p_border,
        int32_t p_border_width,
        int32_t p_radius,
        float p_margin_horizontal,
        float p_margin_vertical) {
    Ref<StyleBoxFlat> style;
    style.instantiate();
    style->set_bg_color(p_background);
    style->set_border_color(p_border);
    style->set_border_width_all(p_border_width);
    style->set_corner_radius_all(p_radius);
    style->set_content_margin(SIDE_LEFT, p_margin_horizontal);
    style->set_content_margin(SIDE_RIGHT, p_margin_horizontal);
    style->set_content_margin(SIDE_TOP, p_margin_vertical);
    style->set_content_margin(SIDE_BOTTOM, p_margin_vertical);
    return style;
}

static String resolve_theme_block_texture(const Ref<MnmsThemeResource> &p_theme, const String &p_block_type) {
    if (p_theme.is_null()) {
        return "";
    }

    Dictionary block_map = p_theme->get_block_texture_map();
    if (!block_map.has(p_block_type)) {
        return "";
    }

    Dictionary metadata = p_theme->get_metadata();
    if (!metadata.has("source_theme_path")) {
        return "";
    }

    const String theme_root = String(metadata["source_theme_path"]).get_base_dir();
    return theme_root.path_join(String(block_map[p_block_type]));
}

static Ref<Texture2D> build_medal_texture(const String &p_path, int32_t p_medal) {
    if (p_medal <= 0) {
        return Ref<Texture2D>();
    }

    Ref<Texture2D> source_texture = load_texture_if_exists(p_path);
    if (source_texture.is_null()) {
        return Ref<Texture2D>();
    }

    const int32_t medal_index = CLAMP(p_medal - 1, 0, 2);
    Ref<AtlasTexture> atlas_texture;
    atlas_texture.instantiate();
    atlas_texture->set_atlas(source_texture);
    atlas_texture->set_region(Rect2(medal_index * 30.0, 0.0, 30.0, 30.0));
    return atlas_texture;
}

static String parse_assignment_value_local(const String &p_line) {
    const int64_t eq = p_line.find("=");
    if (eq < 0) {
        return "";
    }
    String value = p_line.substr(eq + 1, p_line.length()).strip_edges();
    if (value.begins_with("\"") && value.ends_with("\"") && value.length() >= 2) {
        value = value.substr(1, value.length() - 2);
    }
    return value;
}

static String parse_single_call_argument(const String &p_line, const String &p_call_name) {
    const String prefix = p_call_name + String("(");
    if (!p_line.begins_with(prefix) || !p_line.ends_with(String(")"))) {
        return "";
    }

    String value = p_line.substr(prefix.length(), p_line.length() - prefix.length() - 1).strip_edges();
    if (value.begins_with("\"") && value.ends_with("\"") && value.length() >= 2) {
        value = value.substr(1, value.length() - 2);
    }
    return value;
}

static String sanitize_replay_token(String p_value) {
    p_value = p_value.to_lower().strip_edges();
    String out;
    bool last_separator = false;
    for (int64_t i = 0; i < p_value.length(); i++) {
        const char32_t c = p_value[i];
        const bool keep = (c >= U'a' && c <= U'z') || (c >= U'0' && c <= U'9');
        if (keep) {
            out += String::chr(c);
            last_separator = false;
        } else if (!last_separator) {
            out += "_";
            last_separator = true;
        }
    }

    while (out.begins_with("_")) {
        out = out.substr(1);
    }
    while (out.ends_with("_")) {
        out = out.left(out.length() - 1);
    }
    return out.is_empty() ? String("replay") : out;
}

static String format_seconds_as_hms(int32_t p_seconds) {
    const int32_t hours = p_seconds / 3600;
    const int32_t minutes = (p_seconds / 60) % 60;
    const int32_t seconds = p_seconds % 60;
    String out;
    if (hours < 10) {
        out += "0";
    }
    out += String::num_int64(hours) + ":";
    if (minutes < 10) {
        out += "0";
    }
    out += String::num_int64(minutes) + ":";
    if (seconds < 10) {
        out += "0";
    }
    out += String::num_int64(seconds);
    return out;
}

static const int32_t CLASSIC_VALIDATION_LEVELS[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
static const int32_t CLASSIC_VALIDATION_LEVEL_COUNT = sizeof(CLASSIC_VALIDATION_LEVELS) / sizeof(CLASSIC_VALIDATION_LEVELS[0]);

static Dictionary parse_music_definition(const String &p_music_definition_path) {
    Dictionary definition;
    Ref<FileAccess> file = FileAccess::open(p_music_definition_path, FileAccess::READ);
    if (file.is_null()) {
        return definition;
    }

    const String music_root = p_music_definition_path.get_base_dir();
    while (!file->eof_reached()) {
        String line = file->get_line().strip_edges();
        if (line.is_empty() || line.begins_with("#")) {
            continue;
        }
        if (line.begins_with("file=")) {
            const String file_name = parse_assignment_value_local(line);
            if (!file_name.is_empty()) {
                definition["file"] = music_root.path_join(file_name);
            }
        } else if (line.begins_with("loopfile=")) {
            const String loop_file_name = parse_assignment_value_local(line);
            if (!loop_file_name.is_empty()) {
                definition["loopfile"] = music_root.path_join(loop_file_name);
            }
        }
    }

    return definition;
}

static Array parse_music_list_file(const String &p_music_list_path) {
    Array music_paths;
    Ref<FileAccess> file = FileAccess::open(p_music_list_path, FileAccess::READ);
    if (file.is_null()) {
        return music_paths;
    }

    const String music_root = p_music_list_path.get_base_dir();
    while (!file->eof_reached()) {
        String line = file->get_line().strip_edges();
        if (line.is_empty() || line.begins_with("#")) {
            continue;
        }

        const String music_file = parse_single_call_argument(line, "musicfile");
        if (music_file.is_empty()) {
            continue;
        }

        const Dictionary definition = parse_music_definition(music_root.path_join(music_file));
        if (definition.has("file")) {
            music_paths.push_back(String(definition["file"]));
        }
    }

    return music_paths;
}

} // namespace

void MnmsGameDirector::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_set_title_visible", "visible"), &MnmsGameDirector::_set_title_visible);
    ClassDB::bind_method(D_METHOD("_on_title_item_selected", "index"), &MnmsGameDirector::_on_title_item_selected);
    ClassDB::bind_method(D_METHOD("_on_title_item_activated", "index"), &MnmsGameDirector::_on_title_item_activated);
    ClassDB::bind_method(D_METHOD("_on_checkpoint_activated"), &MnmsGameDirector::_on_checkpoint_activated);
    ClassDB::bind_method(D_METHOD("_on_collectable_collected"), &MnmsGameDirector::_on_collectable_collected);
    ClassDB::bind_method(D_METHOD("_on_level_completed", "level_name"), &MnmsGameDirector::_on_level_completed);
    ClassDB::bind_method(D_METHOD("_on_level_failed"), &MnmsGameDirector::_on_level_failed);
    ClassDB::bind_method(D_METHOD("_on_runtime_state_changed", "elapsed_ticks", "recordings_used", "target_time", "target_recordings"), &MnmsGameDirector::_on_runtime_state_changed);
    ClassDB::bind_method(D_METHOD("_on_notification_changed", "message"), &MnmsGameDirector::_on_notification_changed);
    ClassDB::bind_method(D_METHOD("_on_swap_activated"), &MnmsGameDirector::_on_swap_activated);
    ClassDB::bind_method(D_METHOD("_on_selector_pack_selected", "index"), &MnmsGameDirector::_on_selector_pack_selected);
    ClassDB::bind_method(D_METHOD("_on_selector_pack_activated", "index"), &MnmsGameDirector::_on_selector_pack_activated);
    ClassDB::bind_method(D_METHOD("_on_selector_level_selected", "index"), &MnmsGameDirector::_on_selector_level_selected);
    ClassDB::bind_method(D_METHOD("_on_selector_level_activated", "index"), &MnmsGameDirector::_on_selector_level_activated);
    ClassDB::bind_method(D_METHOD("_on_selector_play_pressed"), &MnmsGameDirector::_on_selector_play_pressed);
    ClassDB::bind_method(D_METHOD("_on_interlevel_next_pressed"), &MnmsGameDirector::_on_interlevel_next_pressed);
    ClassDB::bind_method(D_METHOD("_on_interlevel_restart_pressed"), &MnmsGameDirector::_on_interlevel_restart_pressed);
    ClassDB::bind_method(D_METHOD("_on_interlevel_save_replay_pressed"), &MnmsGameDirector::_on_interlevel_save_replay_pressed);
    ClassDB::bind_method(D_METHOD("_on_interlevel_select_pressed"), &MnmsGameDirector::_on_interlevel_select_pressed);
    ClassDB::bind_method(D_METHOD("_on_pause_resume_pressed"), &MnmsGameDirector::_on_pause_resume_pressed);
    ClassDB::bind_method(D_METHOD("_on_pause_restart_pressed"), &MnmsGameDirector::_on_pause_restart_pressed);
    ClassDB::bind_method(D_METHOD("_on_pause_checkpoint_pressed"), &MnmsGameDirector::_on_pause_checkpoint_pressed);
    ClassDB::bind_method(D_METHOD("_on_pause_select_pressed"), &MnmsGameDirector::_on_pause_select_pressed);
    ClassDB::bind_method(D_METHOD("_on_pause_save_replay_pressed"), &MnmsGameDirector::_on_pause_save_replay_pressed);
    ClassDB::bind_method(D_METHOD("_on_pause_options_pressed"), &MnmsGameDirector::_on_pause_options_pressed);
    ClassDB::bind_method(D_METHOD("_on_pause_help_pressed"), &MnmsGameDirector::_on_pause_help_pressed);
    ClassDB::bind_method(D_METHOD("_on_pause_stats_pressed"), &MnmsGameDirector::_on_pause_stats_pressed);
    ClassDB::bind_method(D_METHOD("_on_modal_item_selected", "index"), &MnmsGameDirector::_on_modal_item_selected);
    ClassDB::bind_method(D_METHOD("_on_modal_item_activated", "index"), &MnmsGameDirector::_on_modal_item_activated);
    ClassDB::bind_method(D_METHOD("_on_modal_back_pressed"), &MnmsGameDirector::_on_modal_back_pressed);
    ClassDB::bind_method(D_METHOD("_on_music_finished"), &MnmsGameDirector::_on_music_finished);
}

MnmsGameDirector::MnmsGameDirector() {
    level_runner = nullptr;
    replay_system = nullptr;
    player_shadow_system = nullptr;
    ui_layer = nullptr;
    title_root = nullptr;
    title_panel = nullptr;
    title_label = nullptr;
    title_menu_list = nullptr;
    title_help_label = nullptr;
    title_logo_rect = nullptr;
    title_credits_rect = nullptr;
    title_statistics_rect = nullptr;
    selector_root = nullptr;
    selector_panel = nullptr;
    selector_title_label = nullptr;
    selector_help_label = nullptr;
    selector_progress_label = nullptr;
    selector_details_label = nullptr;
    selector_pack_list = nullptr;
    selector_level_list = nullptr;
    selector_play_button = nullptr;
    selector_back_button = nullptr;
    interlevel_root = nullptr;
    interlevel_panel = nullptr;
    interlevel_title_label = nullptr;
    interlevel_stats_label = nullptr;
    interlevel_help_label = nullptr;
    interlevel_medal_rect = nullptr;
    interlevel_next_button = nullptr;
    interlevel_restart_button = nullptr;
    interlevel_save_replay_button = nullptr;
    interlevel_select_button = nullptr;
    pause_root = nullptr;
    pause_panel = nullptr;
    pause_title_label = nullptr;
    pause_help_label = nullptr;
    pause_resume_button = nullptr;
    pause_restart_button = nullptr;
    pause_checkpoint_button = nullptr;
    pause_select_button = nullptr;
    pause_save_replay_button = nullptr;
    pause_options_button = nullptr;
    pause_help_button = nullptr;
    pause_stats_button = nullptr;
    modal_root = nullptr;
    modal_panel = nullptr;
    modal_title_label = nullptr;
    modal_body_label = nullptr;
    modal_help_label = nullptr;
    modal_list = nullptr;
    modal_back_button = nullptr;
    status_label = nullptr;
    notification_label = nullptr;
    sfx_player = nullptr;
    music_player = nullptr;

    source_data_path = "res://mnms_source_data";
    converted_root_path = "res://mnms_converted";
    progress_file_path = "user://mnms_progress.cfg";
    replay_directory_path = "user://replays";
    current_pack_music_list = "";
    last_saved_replay_path = "";
    last_loaded_replay_path = "";
    default_pack_id = "classic";
    current_pack_id = "";
    current_level_index = -1;
    current_unlocked_level_count = 1;
    selector_unlocked_level_count = 1;
    selected_pack_index = 0;
    selected_level_index = 0;
    last_completed_time = -1;
    last_completed_recordings = -1;
    last_old_best_time = -1;
    last_old_best_recordings = -1;
    last_completed_medal = 0;
    last_old_best_medal = 0;
    gameplay_music_index = 0;
    selector_visible = false;
    title_visible = true;
    interlevel_visible = false;
    pause_visible = false;
    restart_key_down = false;
    load_checkpoint_key_down = false;
    record_key_down = false;
    cancel_record_key_down = false;
    save_replay_key_down = false;
    shadow_view_key_down = false;
    replay_restart_key_down = false;
    replay_stop_key_down = false;
    pause_key_down = false;
    validation_prev_key_down = false;
    validation_next_key_down = false;
    title_up_key_down = false;
    title_down_key_down = false;
    title_select_key_down = false;
    title_cancel_key_down = false;
    selector_toggle_key_down = false;
    selector_left_key_down = false;
    selector_right_key_down = false;
    selector_cancel_key_down = false;
    current_music_mode = "";
    menu_music_intro_path = "";
    menu_music_loop_path = "";
    active_music_stream_path = "";
    modal_screen = "";
    pending_rebind_action = "";
    active_music_is_menu_loop = false;
    modal_from_pause = false;
    play_time_accumulator = 0.0;
    stats_save_accumulator = 0.0;
}

MnmsGameDirector::~MnmsGameDirector() {}

void MnmsGameDirector::_ready() {
    replay_system = memnew(MnmsReplaySystem);
    replay_system->set_name("ReplaySystem");
    add_child(replay_system);

    player_shadow_system = memnew(MnmsPlayerShadowSystem);
    player_shadow_system->set_name("PlayerShadowSystem");
    add_child(player_shadow_system);

    level_runner = memnew(MnmsLevelRunner);
    level_runner->set_name("LevelRunner");
    add_child(level_runner);
    level_runner->connect("level_completed", Callable(this, "_on_level_completed"));
    level_runner->connect("level_failed", Callable(this, "_on_level_failed"));
    level_runner->connect("checkpoint_activated", Callable(this, "_on_checkpoint_activated"));
    level_runner->connect("collectable_collected", Callable(this, "_on_collectable_collected"));
    level_runner->connect("runtime_state_changed", Callable(this, "_on_runtime_state_changed"));
    level_runner->connect("notification_changed", Callable(this, "_on_notification_changed"));
    level_runner->connect("swap_activated", Callable(this, "_on_swap_activated"));

    sfx_player = memnew(AudioStreamPlayer);
    sfx_player->set_name("SfxPlayer");
    add_child(sfx_player);

    music_player = memnew(AudioStreamPlayer);
    music_player->set_name("MusicPlayer");
    music_player->connect("finished", Callable(this, "_on_music_finished"));
    add_child(music_player);

    set_process_mode(Node::PROCESS_MODE_ALWAYS);
    _load_music_library();

    if (!_bootstrap_pipeline()) {
        return;
    }

    _load_progress();
    _ensure_input_actions();
    _ensure_title_ui();
    _ensure_selector_ui();
    _ensure_interlevel_ui();
    _ensure_pause_ui();
    _ensure_modal_ui();
    _apply_common_ui_style(ui_layer);
    set_process(true);
    set_process_input(true);
    _refresh_recent_replays();
    _refresh_achievements();
    _update_audio_settings();

    if (!available_pack_ids.is_empty()) {
        selected_pack_index = CLAMP(selected_pack_index, 0, available_pack_ids.size() - 1);
        _load_selector_pack(String(available_pack_ids[selected_pack_index]));
    }
    _set_title_visible(true);
    _refresh_music_state();
}

bool MnmsGameDirector::_bootstrap_pipeline() {
    if (!_scan_available_packs()) {
        UtilityFunctions::printerr("MNMS: aucun pack converti trouve sous ", converted_root_path, ". Lancez res://tools/rebuild_mnms_converted.tscn pour generer les ressources converties.");
        return false;
    }
    return true;
}

void MnmsGameDirector::_ensure_ui_style_resources() {
    if (ui_body_font.is_null()) {
        ui_body_font = load_font_if_exists(source_data_path.path_join("font/DejaVuSansCondensed.ttf"));
    }
    if (ui_title_font.is_null()) {
        ui_title_font = load_font_if_exists(source_data_path.path_join("font/knewave.ttf"));
    }
    if (ui_title_font.is_null()) {
        ui_title_font = ui_body_font;
    }
}

void MnmsGameDirector::_apply_common_ui_style(Node *p_root) {
    if (p_root == nullptr) {
        return;
    }

    _ensure_ui_style_resources();

    const Color panel_background(0.07, 0.10, 0.15, 0.94);
    const Color panel_border(0.24, 0.34, 0.45, 0.96);
    const Color panel_background_alt(0.10, 0.14, 0.20, 0.96);
    const Color button_background(0.13, 0.20, 0.27, 0.98);
    const Color button_hover(0.18, 0.28, 0.37, 0.98);
    const Color button_pressed(0.21, 0.56, 0.55, 0.98);
    const Color button_disabled(0.12, 0.14, 0.17, 0.86);
    const Color accent_background(0.91, 0.59, 0.26, 0.99);
    const Color accent_hover(0.96, 0.68, 0.33, 0.99);
    const Color accent_pressed(0.84, 0.47, 0.18, 0.99);
    const Color text_color(0.96, 0.97, 0.94, 1.0);
    const Color text_muted(0.74, 0.80, 0.86, 1.0);
    const Color accent_text(0.08, 0.10, 0.12, 1.0);
    const Color selection_color(0.23, 0.58, 0.57, 0.95);

    Control *control = Object::cast_to<Control>(p_root);
    if (control != nullptr) {
        const String node_name = String(control->get_name());

        if (PanelContainer *panel = Object::cast_to<PanelContainer>(control)) {
            Ref<StyleBoxFlat> panel_style = make_flat_stylebox(panel_background, panel_border, 2, 18, 18.0f, 18.0f);
            if (node_name.findn("TitlePanel") >= 0 || node_name.findn("SelectorPanel") >= 0 || node_name.findn("ModalPanel") >= 0) {
                panel_style = make_flat_stylebox(panel_background_alt, panel_border, 2, 22, 20.0f, 20.0f);
            }
            panel->add_theme_stylebox_override("panel", panel_style);
        } else if (Button *button = Object::cast_to<Button>(control)) {
            const bool accent_button = node_name.findn("Play") >= 0 || node_name.findn("Next") >= 0 ||
                    node_name.findn("Resume") >= 0 || node_name.findn("SaveReplay") >= 0;
            button->add_theme_font_override("font", ui_body_font);
            button->add_theme_font_size_override("font_size", 17);
            button->add_theme_color_override("font_color", accent_button ? accent_text : text_color);
            button->add_theme_color_override("font_hover_color", accent_button ? accent_text : text_color);
            button->add_theme_color_override("font_pressed_color", accent_button ? accent_text : text_color);
            button->add_theme_color_override("font_focus_color", accent_button ? accent_text : text_color);
            button->add_theme_color_override("font_disabled_color", Color(0.56, 0.60, 0.64, 0.85));
            button->add_theme_stylebox_override("normal",
                    make_flat_stylebox(accent_button ? accent_background : button_background, panel_border, 1, 14, 18.0f, 10.0f));
            button->add_theme_stylebox_override("hover",
                    make_flat_stylebox(accent_button ? accent_hover : button_hover, selection_color, 2, 14, 18.0f, 10.0f));
            button->add_theme_stylebox_override("pressed",
                    make_flat_stylebox(accent_button ? accent_pressed : button_pressed, selection_color, 2, 14, 18.0f, 10.0f));
            button->add_theme_stylebox_override("focus",
                    make_flat_stylebox(Color(0.0, 0.0, 0.0, 0.0), selection_color, 2, 16, 18.0f, 10.0f));
            button->add_theme_stylebox_override("disabled",
                    make_flat_stylebox(button_disabled, panel_border, 1, 14, 18.0f, 10.0f));
        } else if (ItemList *list = Object::cast_to<ItemList>(control)) {
            list->add_theme_font_override("font", ui_body_font);
            list->add_theme_font_size_override("font_size", 17);
            list->add_theme_color_override("font_color", text_color);
            list->add_theme_color_override("font_selected_color", text_color);
            list->add_theme_color_override("guide_color", Color(0.22, 0.28, 0.35, 0.85));
            list->add_theme_stylebox_override("panel",
                    make_flat_stylebox(panel_background, panel_border, 2, 16, 14.0f, 12.0f));
            list->add_theme_stylebox_override("focus",
                    make_flat_stylebox(Color(0.0, 0.0, 0.0, 0.0), selection_color, 2, 18, 14.0f, 12.0f));
            list->add_theme_color_override("selection_fill", selection_color);
            list->add_theme_color_override("selection_stroke", Color(0.96, 0.97, 0.94, 0.30));
        } else if (Label *label = Object::cast_to<Label>(control)) {
            const bool title_label = node_name.findn("Title") >= 0 && node_name.findn("Help") < 0;
            const bool help_label = node_name.findn("Help") >= 0;
            const bool body_label = node_name.findn("Body") >= 0 || node_name.findn("Stats") >= 0 ||
                    node_name.findn("Progress") >= 0 || node_name.findn("Details") >= 0;
            const bool notification = node_name.findn("Notification") >= 0;
            label->add_theme_font_override("font", title_label ? ui_title_font : ui_body_font);
            label->add_theme_font_size_override("font_size", title_label ? 28 : (help_label ? 15 : (body_label ? 16 : 18)));
            label->add_theme_color_override("font_color",
                    notification ? Color(0.96, 0.82, 0.50, 1.0) : (help_label ? text_muted : text_color));
            if (notification) {
                label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
            }
        } else if (TextureRect *texture_rect = Object::cast_to<TextureRect>(control)) {
            texture_rect->set_modulate(Color(1.0, 1.0, 1.0, 0.96));
        }
    }

    Array children = p_root->get_children();
    for (int32_t i = 0; i < children.size(); i++) {
        Node *child = Object::cast_to<Node>(children[i]);
        if (child != nullptr) {
            _apply_common_ui_style(child);
        }
    }
}

bool MnmsGameDirector::_load_first_level() {
    if (available_pack_ids.is_empty()) {
        return false;
    }

    bool has_saved_selection = selected_pack_index >= 0 && selected_pack_index < available_pack_ids.size();
    if (!has_saved_selection) {
        selected_pack_index = 0;
        for (int i = 0; i < available_pack_ids.size(); i++) {
            if (String(available_pack_ids[i]) == default_pack_id) {
                selected_pack_index = i;
                break;
            }
        }
    }

    if (!_load_pack(String(available_pack_ids[selected_pack_index]))) {
        return false;
    }

    _load_selector_pack(String(available_pack_ids[selected_pack_index]));
    if (selected_level_index < 0 || selected_level_index >= current_pack_levels.size()) {
        selected_level_index = 0;
    }
    if (current_unlocked_level_count > 0) {
        selected_level_index = MIN(selected_level_index, current_unlocked_level_count - 1);
    }

    selector_visible = false;
    const bool ok = _load_selected_level();
    _refresh_selector_ui();
    return ok;
}

void MnmsGameDirector::_ensure_title_ui() {
    if (ui_layer == nullptr) {
        ui_layer = memnew(CanvasLayer);
        ui_layer->set_name("UiLayer");
        add_child(ui_layer);
    }
    if (title_root != nullptr) {
        return;
    }

    title_root = memnew(Control);
    title_root->set_name("TitleRoot");
    title_root->set_anchors_preset(Control::PRESET_FULL_RECT);
    ui_layer->add_child(title_root);

    title_panel = memnew(PanelContainer);
    title_panel->set_name("TitlePanel");
    title_panel->set_position(Vector2(180, 28));
    title_panel->set_size(Vector2(440, 540));
    title_root->add_child(title_panel);

    Control *content = memnew(Control);
    content->set_name("TitleContent");
    content->set_custom_minimum_size(Vector2(440, 540));
    title_panel->add_child(content);

    title_logo_rect = memnew(TextureRect);
    title_logo_rect->set_name("TitleLogo");
    title_logo_rect->set_position(Vector2(36, 24));
    title_logo_rect->set_size(Vector2(368, 144));
    title_logo_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
    title_logo_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
    content->add_child(title_logo_rect);

    title_label = memnew(Label);
    title_label->set_name("TitleLabel");
    title_label->set_position(Vector2(24, 176));
    title_label->set_size(Vector2(392, 32));
    title_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    title_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
    content->add_child(title_label);

    title_menu_list = memnew(ItemList);
    title_menu_list->set_name("TitleMenuList");
    title_menu_list->set_position(Vector2(104, 228));
    title_menu_list->set_size(Vector2(232, 156));
    title_menu_list->set_focus_mode(Control::FOCUS_ALL);
    title_menu_list->set_select_mode(ItemList::SELECT_SINGLE);
    title_menu_list->set_fixed_column_width(232);
    title_menu_list->set_same_column_width(true);
    title_menu_list->set_allow_reselect(true);
    title_menu_list->connect("item_selected", Callable(this, "_on_title_item_selected"));
    title_menu_list->connect("item_activated", Callable(this, "_on_title_item_activated"));
    content->add_child(title_menu_list);

    title_help_label = memnew(Label);
    title_help_label->set_name("TitleHelpLabel");
    title_help_label->set_position(Vector2(24, 406));
    title_help_label->set_size(Vector2(392, 44));
    title_help_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    title_help_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
    title_help_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    content->add_child(title_help_label);

    title_credits_rect = memnew(TextureRect);
    title_credits_rect->set_name("TitleCreditsIcon");
    title_credits_rect->set_position(Vector2(320, 472));
    title_credits_rect->set_size(Vector2(40, 40));
    title_credits_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
    title_credits_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
    content->add_child(title_credits_rect);

    title_statistics_rect = memnew(TextureRect);
    title_statistics_rect->set_name("TitleStatisticsIcon");
    title_statistics_rect->set_position(Vector2(368, 472));
    title_statistics_rect->set_size(Vector2(40, 40));
    title_statistics_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
    title_statistics_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
    content->add_child(title_statistics_rect);

    _refresh_title_ui();
}

bool MnmsGameDirector::_scan_available_packs() {
    available_pack_ids.clear();
    const String packs_root = converted_root_path.path_join("packs");
    Ref<DirAccess> dir = DirAccess::open(packs_root);
    if (dir.is_null()) {
        return false;
    }
    if (dir->list_dir_begin() != OK) {
        return false;
    }

    String entry = dir->get_next();
    while (!entry.is_empty()) {
        if (!dir->current_is_dir() && entry.ends_with("_pack.tres")) {
            available_pack_ids.push_back(entry.trim_suffix("_pack.tres"));
        }
        entry = dir->get_next();
    }
    dir->list_dir_end();
    available_pack_ids.sort();
    return !available_pack_ids.is_empty();
}

bool MnmsGameDirector::_load_pack(const String &p_pack_id) {
    const String pack_path = converted_root_path.path_join("packs").path_join(p_pack_id + String("_pack.tres"));
    Ref<MnmsPackResource> pack = ResourceLoader::get_singleton()->load(pack_path);
    if (pack.is_null()) {
        UtilityFunctions::printerr("MNMS: pack introuvable: ", pack_path);
        return false;
    }

    current_pack_levels = pack->get_levels();
    if (p_pack_id == "classic" && current_pack_levels.size() > MAX_CLASSIC_LEVEL_COUNT) {
        current_pack_levels.resize(MAX_CLASSIC_LEVEL_COUNT);
    }
    current_pack_id = p_pack_id;
    current_unlocked_level_count = _get_unlocked_level_count(p_pack_id);
    if (current_pack_levels.is_empty()) {
        UtilityFunctions::printerr("MNMS: aucun niveau dans le pack: ", p_pack_id);
        return false;
    }

    current_unlocked_level_count = CLAMP(current_unlocked_level_count, 1, current_pack_levels.size());
    const String pack_theme_id = _resolve_pack_theme_id(pack);
    if (!_load_theme(pack_theme_id)) {
        return false;
    }
    pack_theme = current_theme;
    current_pack_music_list = pack->get_music_list();
    _load_music_library();
    _refresh_music_state();
    return true;
}

bool MnmsGameDirector::_load_selector_pack(const String &p_pack_id) {
    const String pack_path = converted_root_path.path_join("packs").path_join(p_pack_id + String("_pack.tres"));
    selector_pack_resource = ResourceLoader::get_singleton()->load(pack_path);
    if (selector_pack_resource.is_null()) {
        UtilityFunctions::printerr("MNMS: pack de selection introuvable: ", pack_path);
        selector_pack_levels.clear();
        selector_unlocked_level_count = 1;
        return false;
    }

    selector_pack_levels = selector_pack_resource->get_levels();
    if (p_pack_id == "classic" && selector_pack_levels.size() > MAX_CLASSIC_LEVEL_COUNT) {
        selector_pack_levels.resize(MAX_CLASSIC_LEVEL_COUNT);
    }
    selector_unlocked_level_count = CLAMP(_get_unlocked_level_count(p_pack_id), 1, MAX(1, selector_pack_levels.size()));
    if (selector_pack_levels.is_empty()) {
        selected_level_index = 0;
        return true;
    }

    if (selected_level_index < 0 || selected_level_index >= selector_pack_levels.size()) {
        selected_level_index = 0;
    }
    selected_level_index = MIN(selected_level_index, selector_unlocked_level_count - 1);
    return true;
}

bool MnmsGameDirector::_load_theme(const String &p_theme_id) {
    String resolved_theme_id = p_theme_id;
    if (resolved_theme_id.is_empty()) {
        resolved_theme_id = "classic";
    }

    const String theme_path = converted_root_path.path_join("themes").path_join(resolved_theme_id + String(".tres"));
    current_theme = ResourceLoader::get_singleton()->load(theme_path);
    if (current_theme.is_null() && resolved_theme_id != "classic") {
        current_theme = ResourceLoader::get_singleton()->load(converted_root_path.path_join("themes").path_join("classic.tres"));
    }
    if (current_theme.is_null()) {
        UtilityFunctions::printerr("MNMS: theme introuvable: ", theme_path);
        return false;
    }
    return true;
}

bool MnmsGameDirector::_load_level_at_index(int32_t p_level_index, bool p_allow_locked, bool p_save_progress) {
    if (level_runner == nullptr || p_level_index < 0 || p_level_index >= current_pack_levels.size()) {
        return false;
    }
    if (!p_allow_locked && p_level_index >= current_unlocked_level_count) {
        UtilityFunctions::print("MNMS: niveau verrouille -> ", p_level_index);
        return false;
    }

    Ref<MnmsLevelResource> level = current_pack_levels[p_level_index];
    if (level.is_null()) {
        UtilityFunctions::printerr("MNMS: niveau invalide dans le pack.");
        return false;
    }

    current_theme = pack_theme;
    Ref<MnmsThemeResource> level_theme = current_theme;
    Dictionary level_metadata = level->get_metadata();
    if (level_metadata.has("theme")) {
        const String level_theme_id = String(level_metadata["theme"]).to_lower();
        if (!level_theme_id.is_empty()) {
            _load_theme(level_theme_id);
            level_theme = current_theme;
        }
    }

    current_theme = level_theme;
    current_level_index = p_level_index;
    selected_level_index = p_level_index;
    level_runner->set_theme(level_theme);
    level_runner->load_level(level);
    _wire_runtime_systems();
    if (p_save_progress) {
        _save_progress();
    }
    _refresh_selector_ui();
    _refresh_status_ui(level_runner->get_elapsed_ticks(), level_runner->get_recordings_used(), level_runner->get_target_time(), level_runner->get_target_recordings());
    UtilityFunctions::print("MNMS: niveau charge -> ", level->get_level_name());
    return true;
}

int32_t MnmsGameDirector::_find_current_classic_validation_slot() const {
    if (current_pack_id != "classic" || current_level_index < 0) {
        return -1;
    }

    for (int32_t i = 0; i < CLASSIC_VALIDATION_LEVEL_COUNT; i++) {
        if (CLASSIC_VALIDATION_LEVELS[i] == current_level_index) {
            return i;
        }
    }
    return -1;
}

bool MnmsGameDirector::_jump_to_classic_validation_level(int32_t p_direction) {
    if (available_pack_ids.is_empty()) {
        return false;
    }

    int32_t classic_pack_index = -1;
    for (int32_t i = 0; i < available_pack_ids.size(); i++) {
        if (String(available_pack_ids[i]) == "classic") {
            classic_pack_index = i;
            break;
        }
    }
    if (classic_pack_index < 0) {
        return false;
    }

    int32_t slot = _find_current_classic_validation_slot();
    if (slot < 0) {
        slot = p_direction < 0 ? CLASSIC_VALIDATION_LEVEL_COUNT - 1 : 0;
    } else {
        slot += p_direction < 0 ? -1 : 1;
        if (slot < 0) {
            slot = CLASSIC_VALIDATION_LEVEL_COUNT - 1;
        } else if (slot >= CLASSIC_VALIDATION_LEVEL_COUNT) {
            slot = 0;
        }
    }

    selected_pack_index = classic_pack_index;
    current_pack_id = "classic";
    if (!_load_selector_pack("classic")) {
        return false;
    }
    if (!_load_pack("classic")) {
        return false;
    }

    selected_level_index = CLASSIC_VALIDATION_LEVELS[slot];
    _set_title_visible(false);
    _set_selector_visible(false);
    _set_interlevel_visible(false);
    _set_pause_visible(false);
    return _load_level_at_index(CLASSIC_VALIDATION_LEVELS[slot], true, false);
}

String MnmsGameDirector::_get_classic_validation_status() const {
    const int32_t slot = _find_current_classic_validation_slot();
    if (slot < 0 || level_runner == nullptr || level_runner->get_current_level().is_null()) {
        return "Parcours classic: hors selection golden";
    }

    Ref<MnmsLevelResource> level = level_runner->get_current_level();
    return "Parcours classic: " + String::num_int64(slot + 1) + "/" + String::num_int64(CLASSIC_VALIDATION_LEVEL_COUNT) +
        "  " + level->get_level_name() + "  (F6 precedent / F7 suivant)";
}

bool MnmsGameDirector::_load_selected_level() {
    if (available_pack_ids.is_empty()) {
        return false;
    }
    selected_pack_index = CLAMP(selected_pack_index, 0, available_pack_ids.size() - 1);
    const String pack_id = available_pack_ids[selected_pack_index];
    if (!_load_selector_pack(pack_id)) {
        return false;
    }
    if (!_load_pack(pack_id)) {
        return false;
    }

    if (selector_pack_levels.is_empty()) {
        return false;
    }
    selected_level_index = CLAMP(selected_level_index, 0, selector_pack_levels.size() - 1);
    if (selected_level_index >= current_unlocked_level_count) {
        selected_level_index = MAX(0, current_unlocked_level_count - 1);
    }

    const bool ok = _load_level_at_index(selected_level_index);
    _refresh_selector_ui();
    return ok;
}

void MnmsGameDirector::_load_progress() {
    unlocked_levels_by_pack.clear();
    best_times_by_level.clear();
    best_recordings_by_level.clear();
    options_state.clear();
    stats_state.clear();
    achievements_state.clear();
    controls_by_action.clear();
    Ref<ConfigFile> config;
    config.instantiate();
    if (config->load(progress_file_path) != OK) {
        options_state["music_enabled"] = true;
        options_state["sfx_enabled"] = true;
        options_state["show_status"] = true;
        options_state["show_notifications"] = true;
        return;
    }

    const String saved_pack_id = config->get_value("selection", "pack_id", default_pack_id);
    const int32_t saved_level_index = (int32_t)(int)config->get_value("selection", "level_index", 0);
    selected_level_index = MAX(0, saved_level_index);
    best_times_by_level = config->get_value("records", "best_times", Dictionary());
    best_recordings_by_level = config->get_value("records", "best_recordings", Dictionary());
    options_state = config->get_value("options", "values", Dictionary());
    stats_state = config->get_value("stats", "values", Dictionary());
    achievements_state = config->get_value("achievements", "unlocked", Dictionary());
    controls_by_action = config->get_value("controls", "bindings", Dictionary());
    last_saved_replay_path = config->get_value("replays", "last_saved", String());
    last_loaded_replay_path = config->get_value("replays", "last_loaded", String());

    if (!options_state.has("music_enabled")) {
        options_state["music_enabled"] = true;
    }
    if (!options_state.has("sfx_enabled")) {
        options_state["sfx_enabled"] = true;
    }
    if (!options_state.has("show_status")) {
        options_state["show_status"] = true;
    }
    if (!options_state.has("show_notifications")) {
        options_state["show_notifications"] = true;
    }

    for (int i = 0; i < available_pack_ids.size(); i++) {
        const String pack_id = available_pack_ids[i];
        const int32_t unlocked_count = MAX(1, (int32_t)(int)config->get_value("progress", pack_id, 1));
        unlocked_levels_by_pack[pack_id] = unlocked_count;
        if (pack_id == saved_pack_id) {
            selected_pack_index = i;
        }
    }
}

void MnmsGameDirector::_save_progress() {
    Ref<ConfigFile> config;
    config.instantiate();
    config->set_value("selection", "pack_id", current_pack_id.is_empty() ? String(available_pack_ids[selected_pack_index]) : current_pack_id);
    config->set_value("selection", "level_index", selected_level_index);
    config->set_value("records", "best_times", best_times_by_level);
    config->set_value("records", "best_recordings", best_recordings_by_level);
    config->set_value("options", "values", options_state);
    config->set_value("stats", "values", stats_state);
    config->set_value("achievements", "unlocked", achievements_state);
    config->set_value("controls", "bindings", controls_by_action);
    config->set_value("replays", "last_saved", last_saved_replay_path);
    config->set_value("replays", "last_loaded", last_loaded_replay_path);

    Array keys = unlocked_levels_by_pack.keys();
    for (int i = 0; i < keys.size(); i++) {
        const String pack_id = keys[i];
        config->set_value("progress", pack_id, unlocked_levels_by_pack[pack_id]);
    }
    config->save(progress_file_path);
}

String MnmsGameDirector::_get_level_record_key(const String &p_pack_id, int32_t p_level_index) const {
    return p_pack_id + String(":") + String::num_int64(p_level_index);
}

int32_t MnmsGameDirector::_get_best_time_for_level(const String &p_pack_id, int32_t p_level_index) const {
    const String key = _get_level_record_key(p_pack_id, p_level_index);
    if (best_times_by_level.has(key)) {
        return (int32_t)(int)best_times_by_level[key];
    }
    return -1;
}

int32_t MnmsGameDirector::_get_best_recordings_for_level(const String &p_pack_id, int32_t p_level_index) const {
    const String key = _get_level_record_key(p_pack_id, p_level_index);
    if (best_recordings_by_level.has(key)) {
        return (int32_t)(int)best_recordings_by_level[key];
    }
    return -1;
}

int32_t MnmsGameDirector::_compute_level_medal(int32_t p_time, int32_t p_target_time, int32_t p_recordings, int32_t p_target_recordings) const {
    if (p_time < 0 || p_recordings < 0) {
        return 0;
    }

    int32_t medal = 1;
    if (p_target_time < 0 || p_time <= p_target_time) {
        medal += 1;
    }
    if (p_target_recordings < 0 || p_recordings <= p_target_recordings) {
        medal += 1;
    }
    return medal;
}

int32_t MnmsGameDirector::_get_unlocked_level_count(const String &p_pack_id) const {
    if (unlocked_levels_by_pack.has(p_pack_id)) {
        return MAX(1, (int32_t)(int)unlocked_levels_by_pack[p_pack_id]);
    }
    return 1;
}

void MnmsGameDirector::_set_unlocked_level_count(const String &p_pack_id, int32_t p_unlocked_level_count) {
    unlocked_levels_by_pack[p_pack_id] = MAX(1, p_unlocked_level_count);
}

String MnmsGameDirector::_resolve_pack_theme_id(const Ref<MnmsPackResource> &p_pack) const {
    if (p_pack.is_valid()) {
        const String pack_theme_id = p_pack->get_theme_id();
        if (!pack_theme_id.is_empty()) {
            return pack_theme_id;
        }
        Dictionary metadata = p_pack->get_metadata();
        if (metadata.has("theme")) {
            return String(metadata["theme"]);
        }
        if (p_pack->get_pack_id() == "classic") {
            return "classic";
        }
    }
    return "cloudscape";
}

void MnmsGameDirector::_refresh_recent_replays() {
    recent_replay_files.clear();
    Ref<DirAccess> dir = DirAccess::open(replay_directory_path);
    if (dir.is_null() || dir->list_dir_begin() != OK) {
        return;
    }

    String entry = dir->get_next();
    while (!entry.is_empty()) {
        if (!dir->current_is_dir() && entry.ends_with(".cfg")) {
            recent_replay_files.push_back(replay_directory_path.path_join(entry));
        }
        entry = dir->get_next();
    }
    dir->list_dir_end();
    recent_replay_files.sort();

    if (!last_saved_replay_path.is_empty() && FileAccess::file_exists(last_saved_replay_path)) {
        recent_replay_files.erase(last_saved_replay_path);
        recent_replay_files.insert(0, last_saved_replay_path);
    }
}

void MnmsGameDirector::_ensure_input_actions() {
    InputMap *input_map = InputMap::get_singleton();
    if (input_map == nullptr) {
        return;
    }

    for (int32_t i = 0; i < ACTION_BINDING_COUNT; i++) {
        const MnmsActionBindingDef &binding = ACTION_BINDINGS[i];
        if (!input_map->has_action(binding.action)) {
            input_map->add_action(binding.action);
        }
        const int32_t saved_key = controls_by_action.has(binding.action) ? (int32_t)(int)controls_by_action[binding.action] : (int32_t)binding.default_key;
        _apply_control_binding(binding.action, saved_key);
    }
}

void MnmsGameDirector::_apply_control_binding(const String &p_action, int32_t p_keycode) {
    InputMap *input_map = InputMap::get_singleton();
    if (input_map == nullptr || !input_map->has_action(p_action)) {
        return;
    }

    controls_by_action[p_action] = p_keycode;
    input_map->action_erase_events(p_action);

    Ref<InputEventKey> event;
    event.instantiate();
    event->set_keycode((Key)p_keycode);
    event->set_physical_keycode((Key)p_keycode);
    input_map->action_add_event(p_action, event);

    if (p_action == "mnms_pause" && p_keycode != KEY_P) {
        Ref<InputEventKey> alt_event;
        alt_event.instantiate();
        alt_event->set_keycode(KEY_P);
        alt_event->set_physical_keycode(KEY_P);
        input_map->action_add_event(p_action, alt_event);
    }
    if (p_action == "mnms_cancel_or_stop" && p_keycode != KEY_DELETE) {
        Ref<InputEventKey> alt_event;
        alt_event.instantiate();
        alt_event->set_keycode(KEY_DELETE);
        alt_event->set_physical_keycode(KEY_DELETE);
        input_map->action_add_event(p_action, alt_event);
    }
}

int32_t MnmsGameDirector::_get_bound_keycode(const String &p_action) const {
    if (controls_by_action.has(p_action)) {
        return (int32_t)(int)controls_by_action[p_action];
    }

    for (int32_t i = 0; i < ACTION_BINDING_COUNT; i++) {
        if (p_action == ACTION_BINDINGS[i].action) {
            return (int32_t)ACTION_BINDINGS[i].default_key;
        }
    }
    return KEY_NONE;
}

String MnmsGameDirector::_get_action_label(const String &p_action) const {
    for (int32_t i = 0; i < ACTION_BINDING_COUNT; i++) {
        if (p_action == ACTION_BINDINGS[i].action) {
            return ACTION_BINDINGS[i].label;
        }
    }
    return p_action;
}

String MnmsGameDirector::_get_keycode_label(int32_t p_keycode) const {
    if (p_keycode == KEY_NONE) {
        return "-";
    }
    String label = OS::get_singleton()->get_keycode_string((Key)p_keycode);
    return label.is_empty() ? String::num_int64(p_keycode) : label;
}

bool MnmsGameDirector::_get_option_enabled(const String &p_option_key, bool p_default_value) const {
    if (!options_state.has(p_option_key)) {
        return p_default_value;
    }
    return (bool)options_state[p_option_key];
}

void MnmsGameDirector::_set_option_enabled(const String &p_option_key, bool p_enabled) {
    options_state[p_option_key] = p_enabled;
    _update_audio_settings();
    _refresh_status_ui();
    if (notification_label != nullptr && p_option_key == "show_notifications" && !p_enabled) {
        notification_label->set_visible(false);
    }
    _save_progress();
}

int32_t MnmsGameDirector::_get_stat_value(const String &p_key) const {
    if (!stats_state.has(p_key)) {
        return 0;
    }
    return (int32_t)(int)stats_state[p_key];
}

void MnmsGameDirector::_set_stat_value(const String &p_key, int32_t p_value) {
    stats_state[p_key] = p_value;
}

void MnmsGameDirector::_add_stat(const String &p_key, int32_t p_amount) {
    _set_stat_value(p_key, _get_stat_value(p_key) + p_amount);
    _refresh_achievements();
}

void MnmsGameDirector::_refresh_achievements() {
    for (int32_t i = 0; i < ACHIEVEMENT_COUNT; i++) {
        const MnmsAchievementDef &achievement = ACHIEVEMENTS[i];
        if (_get_stat_value(achievement.stat_key) >= achievement.threshold) {
            _unlock_achievement(achievement.id, achievement.title);
        }
    }
}

void MnmsGameDirector::_unlock_achievement(const String &p_id, const String &p_title) {
    if (achievements_state.has(p_id) && (bool)achievements_state[p_id]) {
        return;
    }

    achievements_state[p_id] = true;
    _play_sfx("achievement.ogg");
    _show_runtime_message("Achievement debloque: " + p_title);
    _save_progress();
}

String MnmsGameDirector::_build_help_text() const {
    String text = "Commandes principales\n";
    text += _get_action_label("ui_left") + ": " + _get_keycode_label(_get_bound_keycode("ui_left"));
    text += "    " + _get_action_label("ui_right") + ": " + _get_keycode_label(_get_bound_keycode("ui_right")) + "\n";
    text += _get_action_label("ui_up") + ": " + _get_keycode_label(_get_bound_keycode("ui_up"));
    text += "    " + _get_action_label("ui_down") + ": " + _get_keycode_label(_get_bound_keycode("ui_down")) + "\n";
    text += _get_action_label("mnms_record_toggle") + ": " + _get_keycode_label(_get_bound_keycode("mnms_record_toggle"));
    text += "    " + _get_action_label("mnms_cancel_or_stop") + ": " + _get_keycode_label(_get_bound_keycode("mnms_cancel_or_stop")) + " / Delete\n";
    text += _get_action_label("mnms_shadow_view") + ": " + _get_keycode_label(_get_bound_keycode("mnms_shadow_view"));
    text += "    " + _get_action_label("mnms_pause") + ": " + _get_keycode_label(_get_bound_keycode("mnms_pause")) + " / P\n";
    text += _get_action_label("mnms_restart") + ": " + _get_keycode_label(_get_bound_keycode("mnms_restart"));
    text += "    " + _get_action_label("mnms_checkpoint") + ": " + _get_keycode_label(_get_bound_keycode("mnms_checkpoint")) + "\n";
    text += _get_action_label("mnms_save_replay") + ": " + _get_keycode_label(_get_bound_keycode("mnms_save_replay"));
    text += "    " + _get_action_label("mnms_replay_restart") + ": " + _get_keycode_label(_get_bound_keycode("mnms_replay_restart")) + "\n";
    text += _get_action_label("mnms_level_select") + ": " + _get_keycode_label(_get_bound_keycode("mnms_level_select"));
    text += "    Golden: F6 / F7\n\n";
    text += "Boucle replay\n";
    text += "- Espace lance un enregistrement, puis relance le shadow.\n";
    text += "- Backspace annule un enregistrement ou stoppe un replay.\n";
    text += "- F5 sauvegarde le replay courant dans user://replays.\n";
    text += "- F8 relance le replay courant depuis le debut.\n\n";
    text += "Checkpoint\n";
    text += "- L charge le checkpoint actif.\n";
    text += "- S'il n'existe pas encore, le niveau recommence depuis le debut.";
    return text;
}

String MnmsGameDirector::_build_credits_text() const {
    String text = "MNMS Godot - mode classique\n";
    text += "Port runtime cible sur le pack classic uniquement.\n\n";
    const String credits_path = source_data_path.path_join("themes").path_join("classic").path_join("Credits.txt");
    Ref<FileAccess> file = FileAccess::open(credits_path, FileAccess::READ);
    if (!file.is_null()) {
        while (!file->eof_reached()) {
            text += file->get_line();
            if (!file->eof_reached()) {
                text += "\n";
            }
        }
    } else {
        text += "Credits introuvables: " + credits_path;
    }
    return text;
}

String MnmsGameDirector::_build_stats_text() const {
    int32_t completed_levels = 0;
    int32_t bronze_medals = 0;
    int32_t silver_medals = 0;
    int32_t gold_medals = 0;

    Array record_keys = best_times_by_level.keys();
    for (int32_t i = 0; i < record_keys.size(); i++) {
        const String key = record_keys[i];
        const PackedStringArray parts = key.split(":");
        if (parts.size() != 2) {
            continue;
        }
        const String pack_id = parts[0];
        const int32_t level_index = parts[1].to_int();
        const String pack_path = converted_root_path.path_join("packs").path_join(pack_id + String("_pack.tres"));
        Ref<MnmsPackResource> pack = ResourceLoader::get_singleton()->load(pack_path);
        if (pack.is_null()) {
            continue;
        }
        Array pack_levels = pack->get_levels();
        if (level_index < 0 || level_index >= pack_levels.size()) {
            continue;
        }
        const Ref<MnmsLevelResource> level = pack_levels[level_index];
        if (level.is_null()) {
            continue;
        }

        completed_levels += 1;
        const int32_t medal = _compute_level_medal(
            _get_best_time_for_level(pack_id, level_index),
            level->get_time_limit(),
            _get_best_recordings_for_level(pack_id, level_index),
            level->get_recordings_count());
        if (medal >= 3) {
            gold_medals += 1;
        } else if (medal == 2) {
            silver_medals += 1;
        } else if (medal == 1) {
            bronze_medals += 1;
        }
    }

    String text = "Progression locale\n";
    text += "Niveaux termines: " + String::num_int64(completed_levels) + "\n";
    text += "Medailles bronze/argent/or: " + String::num_int64(bronze_medals) + " / " + String::num_int64(silver_medals) + " / " + String::num_int64(gold_medals) + "\n";
    text += "Temps de jeu cumule: " + format_seconds_as_hms(_get_stat_value("play_time_seconds")) + "\n\n";
    text += "Statistiques runtime\n";
    text += "Replays sauves: " + String::num_int64(_get_stat_value("replay_saves")) + "\n";
    text += "Replays charges: " + String::num_int64(_get_stat_value("replay_loads")) + "\n";
    text += "Enregistrements lances: " + String::num_int64(_get_stat_value("recordings_started")) + "\n";
    text += "Collectables: " + String::num_int64(_get_stat_value("collectables")) + "\n";
    text += "Swaps: " + String::num_int64(_get_stat_value("swaps")) + "\n";
    text += "Checkpoints actives: " + String::num_int64(_get_stat_value("checkpoint_activations")) + "\n";
    text += "Charges checkpoint/reprise: " + String::num_int64(_get_stat_value("checkpoint_loads")) + "\n";
    text += "Restarts: " + String::num_int64(_get_stat_value("restarts")) + "\n";
    text += "Echecs: " + String::num_int64(_get_stat_value("deaths")) + "\n\n";
    text += "Achievements debloques: ";
    int32_t unlocked = 0;
    for (int32_t i = 0; i < ACHIEVEMENT_COUNT; i++) {
        if (achievements_state.has(ACHIEVEMENTS[i].id) && (bool)achievements_state[ACHIEVEMENTS[i].id]) {
            unlocked += 1;
        }
    }
    text += String::num_int64(unlocked) + "/" + String::num_int64(ACHIEVEMENT_COUNT) + "\n";
    for (int32_t i = 0; i < ACHIEVEMENT_COUNT; i++) {
        const MnmsAchievementDef &achievement = ACHIEVEMENTS[i];
        const bool done = achievements_state.has(achievement.id) && (bool)achievements_state[achievement.id];
        text += done ? "[OK] " : "[ ] ";
        text += String(achievement.title) + " - " + achievement.description;
        if (!done) {
            text += " (" + String::num_int64(_get_stat_value(achievement.stat_key)) + "/" + String::num_int64(achievement.threshold) + ")";
        }
        if (i + 1 < ACHIEVEMENT_COUNT) {
            text += "\n";
        }
    }
    return text;
}

void MnmsGameDirector::_show_runtime_message(const String &p_message) {
    if (notification_label == nullptr || !_get_option_enabled("show_notifications", true)) {
        return;
    }
    notification_label->set_text(p_message);
    notification_label->set_visible(!p_message.is_empty() && !title_visible && !selector_visible && !interlevel_visible && modal_screen.is_empty());
}

void MnmsGameDirector::_update_audio_settings() {
    if (music_player != nullptr) {
        music_player->set_volume_db(_get_option_enabled("music_enabled", true) ? 0.0 : -80.0);
    }
    if (sfx_player != nullptr) {
        sfx_player->set_volume_db(_get_option_enabled("sfx_enabled", true) ? 0.0 : -80.0);
    }
}

bool MnmsGameDirector::_load_replay_from_file(const String &p_path) {
    if (replay_system == nullptr || player_shadow_system == nullptr || p_path.is_empty()) {
        return false;
    }

    Dictionary metadata = replay_system->load_from_file(p_path);
    if (!metadata.has("ok") || !(bool)metadata["ok"]) {
        return false;
    }

    const String pack_id = metadata.has("pack_id") ? String(metadata["pack_id"]) : default_pack_id;
    const int32_t level_index = metadata.has("level_index") ? (int32_t)(int)metadata["level_index"] : 0;

    if (!_load_pack(pack_id)) {
        return false;
    }
    _load_selector_pack(pack_id);
    selected_level_index = level_index;
    if (!_load_level_at_index(level_index, true, false)) {
        return false;
    }

    if (!player_shadow_system->restart_replay()) {
        return false;
    }

    last_loaded_replay_path = p_path;
    _add_stat("replay_loads");
    _save_progress();
    _show_runtime_message("Replay charge: " + p_path.get_file());
    _refresh_status_ui();
    return true;
}

void MnmsGameDirector::_ensure_selector_ui() {
    if (ui_layer == nullptr) {
        ui_layer = memnew(CanvasLayer);
        ui_layer->set_name("UiLayer");
        add_child(ui_layer);
    }
    if (selector_root != nullptr) {
        return;
    }

    selector_root = memnew(Control);
    selector_root->set_name("SelectorRoot");
    selector_root->set_anchors_preset(Control::PRESET_FULL_RECT);
    selector_root->set_visible(false);
    ui_layer->add_child(selector_root);

    selector_panel = memnew(PanelContainer);
    selector_panel->set_name("SelectorPanel");
    selector_panel->set_position(Vector2(80, 56));
    selector_panel->set_size(Vector2(640, 488));
    selector_root->add_child(selector_panel);

    Control *panel_content = memnew(Control);
    panel_content->set_name("SelectorContent");
    panel_content->set_custom_minimum_size(Vector2(640, 488));
    selector_panel->add_child(panel_content);

    selector_title_label = memnew(Label);
    selector_title_label->set_name("SelectorTitle");
    selector_title_label->set_position(Vector2(24, 20));
    selector_title_label->set_size(Vector2(592, 24));
    panel_content->add_child(selector_title_label);

    selector_help_label = memnew(Label);
    selector_help_label->set_name("SelectorHelp");
    selector_help_label->set_position(Vector2(24, 52));
    selector_help_label->set_size(Vector2(592, 48));
    selector_help_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    panel_content->add_child(selector_help_label);

    selector_progress_label = memnew(Label);
    selector_progress_label->set_name("SelectorProgress");
    selector_progress_label->set_position(Vector2(24, 104));
    selector_progress_label->set_size(Vector2(592, 40));
    selector_progress_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    panel_content->add_child(selector_progress_label);

    Label *pack_title = memnew(Label);
    pack_title->set_name("PackTitle");
    pack_title->set_text("Packs");
    pack_title->set_position(Vector2(24, 150));
    pack_title->set_size(Vector2(200, 24));
    panel_content->add_child(pack_title);

    selector_pack_list = memnew(ItemList);
    selector_pack_list->set_name("PackList");
    selector_pack_list->set_position(Vector2(24, 178));
    selector_pack_list->set_size(Vector2(196, 190));
    selector_pack_list->set_focus_mode(Control::FOCUS_ALL);
    selector_pack_list->connect("item_selected", Callable(this, "_on_selector_pack_selected"));
    selector_pack_list->connect("item_activated", Callable(this, "_on_selector_pack_activated"));
    panel_content->add_child(selector_pack_list);

    Label *level_title = memnew(Label);
    level_title->set_name("LevelTitle");
    level_title->set_text("Niveaux");
    level_title->set_position(Vector2(244, 150));
    level_title->set_size(Vector2(372, 24));
    panel_content->add_child(level_title);

    selector_level_list = memnew(ItemList);
    selector_level_list->set_name("LevelList");
    selector_level_list->set_position(Vector2(244, 178));
    selector_level_list->set_size(Vector2(372, 224));
    selector_level_list->set_focus_mode(Control::FOCUS_ALL);
    selector_level_list->set_select_mode(ItemList::SELECT_SINGLE);
    selector_level_list->set_icon_mode(ItemList::ICON_MODE_TOP);
    selector_level_list->set_fixed_icon_size(Vector2i(50, 50));
    selector_level_list->set_max_columns(5);
    selector_level_list->set_same_column_width(true);
    selector_level_list->set_fixed_column_width(66);
    selector_level_list->set_allow_reselect(true);
    selector_level_list->set_wraparound_items(true);
    selector_level_list->connect("item_selected", Callable(this, "_on_selector_level_selected"));
    selector_level_list->connect("item_activated", Callable(this, "_on_selector_level_activated"));
    panel_content->add_child(selector_level_list);

    selector_details_label = memnew(Label);
    selector_details_label->set_name("SelectorDetails");
    selector_details_label->set_position(Vector2(24, 416));
    selector_details_label->set_size(Vector2(592, 56));
    selector_details_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    panel_content->add_child(selector_details_label);

    selector_back_button = memnew(Button);
    selector_back_button->set_name("SelectorBackButton");
    selector_back_button->set_text("Retour");
    selector_back_button->set_position(Vector2(24, 438));
    selector_back_button->set_size(Vector2(120, 32));
    selector_back_button->set_focus_mode(Control::FOCUS_ALL);
    selector_back_button->connect("pressed", Callable(this, "_set_title_visible").bind(true));
    panel_content->add_child(selector_back_button);

    selector_play_button = memnew(Button);
    selector_play_button->set_name("SelectorPlayButton");
    selector_play_button->set_text("Jouer");
    selector_play_button->set_position(Vector2(456, 438));
    selector_play_button->set_size(Vector2(160, 32));
    selector_play_button->set_focus_mode(Control::FOCUS_ALL);
    selector_play_button->connect("pressed", Callable(this, "_on_selector_play_pressed"));
    panel_content->add_child(selector_play_button);

    status_label = memnew(Label);
    status_label->set_name("StatusLabel");
    status_label->set_position(Vector2(16, 16));
    status_label->set_size(Vector2(768, 72));
    status_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    ui_layer->add_child(status_label);

    notification_label = memnew(Label);
    notification_label->set_name("NotificationLabel");
    notification_label->set_position(Vector2(16, 96));
    notification_label->set_size(Vector2(768, 72));
    notification_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    notification_label->set_visible(false);
    ui_layer->add_child(notification_label);

    _refresh_selector_ui();
    _refresh_status_ui();
}

void MnmsGameDirector::_refresh_title_ui() {
    if (title_root == nullptr || title_menu_list == nullptr) {
        return;
    }

    title_root->set_visible(title_visible && modal_screen.is_empty());
    title_label->set_text("Menu principal");
    title_help_label->set_text("Fleches: naviguer    Entree: choisir    Echap: quitter");
    if (title_logo_rect != nullptr) {
        title_logo_rect->set_texture(load_texture_if_exists(source_data_path.path_join("gfx/menu/title.png")));
    }
    if (title_credits_rect != nullptr) {
        title_credits_rect->set_texture(load_texture_if_exists(source_data_path.path_join("gfx/menu/credits.png")));
    }
    if (title_statistics_rect != nullptr) {
        title_statistics_rect->set_texture(load_texture_if_exists(source_data_path.path_join("gfx/menu/statistics.png")));
    }

    title_menu_list->clear();
    if (available_pack_ids.size() <= 1) {
        title_menu_list->add_item("Reprendre");
    }
    title_menu_list->add_item("Choisir un niveau");
    title_menu_list->add_item("Options");
    title_menu_list->add_item("Aide");
    title_menu_list->add_item("Statistiques");
    title_menu_list->add_item("Credits");
    if (!recent_replay_files.is_empty()) {
        title_menu_list->add_item("Relire le dernier replay");
    }
    title_menu_list->add_item("Quitter");
    if (title_menu_list->get_item_count() > 0 && title_menu_list->get_selected_items().is_empty()) {
        title_menu_list->select(0);
    }
}

void MnmsGameDirector::_refresh_selector_ui() {
    if (selector_root == nullptr || selector_pack_list == nullptr || selector_level_list == nullptr) {
        return;
    }

    const bool single_pack_mode = available_pack_ids.size() <= 1;
    Node *pack_title_node = selector_panel != nullptr ? selector_panel->get_node_or_null(NodePath("SelectorContent/PackTitle")) : nullptr;
    Node *level_title_node = selector_panel != nullptr ? selector_panel->get_node_or_null(NodePath("SelectorContent/LevelTitle")) : nullptr;
    Label *pack_title = Object::cast_to<Label>(pack_title_node);
    Label *level_title = Object::cast_to<Label>(level_title_node);

    selector_root->set_visible(selector_visible);
    selector_title_label->set_text(single_pack_mode ? "Mode classique" : "Choix du niveau");
    selector_help_label->set_text(single_pack_mode
        ? "Entree: lancer le niveau    Echap: retour au menu"
        : "Entree: lancer le niveau    Tab: changer de focus    Echap: retour au menu");

    if (pack_title != nullptr) {
        pack_title->set_visible(!single_pack_mode);
    }
    if (level_title != nullptr) {
        level_title->set_text(single_pack_mode ? "Niveaux classiques" : "Niveaux");
        level_title->set_position(single_pack_mode ? Vector2(24, 150) : Vector2(244, 150));
        level_title->set_size(single_pack_mode ? Vector2(592, 24) : Vector2(372, 24));
    }
    selector_pack_list->set_visible(!single_pack_mode);
    selector_pack_list->set_position(single_pack_mode ? Vector2(-1000, -1000) : Vector2(24, 178));
    selector_pack_list->set_size(single_pack_mode ? Vector2(1, 1) : Vector2(196, 190));
    selector_level_list->set_position(single_pack_mode ? Vector2(24, 178) : Vector2(244, 178));
    selector_level_list->set_size(single_pack_mode ? Vector2(592, 224) : Vector2(372, 224));
    selector_level_list->set_max_columns(single_pack_mode ? 8 : 5);
    selector_level_list->set_fixed_column_width(single_pack_mode ? 68 : 66);

    selector_pack_list->clear();
    selector_level_list->clear();

    if (!available_pack_ids.is_empty()) {
        selected_pack_index = CLAMP(selected_pack_index, 0, available_pack_ids.size() - 1);
        for (int i = 0; i < available_pack_ids.size(); i++) {
            const String pack_id = available_pack_ids[i];
            selector_pack_list->add_item(pack_id);
        }
        selector_pack_list->select(selected_pack_index);
    }

    if (!selector_pack_levels.is_empty()) {
        Ref<MnmsThemeResource> selector_theme;
        if (selector_pack_resource.is_valid()) {
            const String selector_theme_id = _resolve_pack_theme_id(selector_pack_resource);
            const String selector_theme_path = converted_root_path.path_join("themes").path_join(selector_theme_id + String(".tres"));
            selector_theme = ResourceLoader::get_singleton()->load(selector_theme_path);
            if (selector_theme.is_null() && selector_theme_id != "classic") {
                selector_theme = ResourceLoader::get_singleton()->load(converted_root_path.path_join("themes").path_join("classic.tres"));
            }
        }
        const String unlocked_texture_path = resolve_theme_block_texture(selector_theme, "Block");
        const String locked_texture_path = resolve_theme_block_texture(selector_theme, "ShadowBlock");
        const Ref<Texture2D> unlocked_texture = unlocked_texture_path.is_empty() ? Ref<Texture2D>() : load_texture_if_exists(unlocked_texture_path);
        const Ref<Texture2D> locked_texture = locked_texture_path.is_empty() ? Ref<Texture2D>() : load_texture_if_exists(locked_texture_path);

        selected_level_index = CLAMP(selected_level_index, 0, selector_pack_levels.size() - 1);
        for (int i = 0; i < selector_pack_levels.size(); i++) {
            const Ref<MnmsLevelResource> level = selector_pack_levels[i];
            const bool unlocked = i < selector_unlocked_level_count;
            selector_level_list->add_item(String::num_int64(i + 1), unlocked ? unlocked_texture : locked_texture, unlocked);
            selector_level_list->set_item_disabled(i, !unlocked);
            selector_level_list->set_item_tooltip(i, level.is_valid() ? level->get_level_name() : String("Niveau invalide"));
            selector_level_list->set_item_icon_modulate(i, unlocked ? Color(1, 1, 1, 1) : Color(0.72, 0.72, 0.8, 1));
            if (level.is_valid()) {
                const int32_t medal = _compute_level_medal(
                    _get_best_time_for_level(String(available_pack_ids[selected_pack_index]), i),
                    level->get_time_limit(),
                    _get_best_recordings_for_level(String(available_pack_ids[selected_pack_index]), i),
                    level->get_recordings_count());
                if (medal >= 3) {
                    selector_level_list->set_item_custom_bg_color(i, Color(0.92, 0.82, 0.25, 0.22));
                } else if (medal >= 2) {
                    selector_level_list->set_item_custom_bg_color(i, Color(0.85, 0.85, 0.9, 0.2));
                } else if (medal >= 1) {
                    selector_level_list->set_item_custom_bg_color(i, Color(0.72, 0.45, 0.22, 0.15));
                }
            }
        }
        selector_level_list->select(selected_level_index);
    }

    String progress_text;
    if (!available_pack_ids.is_empty()) {
        const String selected_pack_id = available_pack_ids[selected_pack_index];
        if (single_pack_mode) {
            progress_text = "Mode actif: classique";
            progress_text += "    Niveaux debloques: " + String::num_int64(selector_unlocked_level_count) + "/" + String::num_int64(selector_pack_levels.size());
        } else {
            progress_text = "Pack actif: " + selected_pack_id;
            progress_text += "    Debloques: " + String::num_int64(selector_unlocked_level_count) + "/" + String::num_int64(selector_pack_levels.size());
        }
    } else {
        progress_text = "Aucun pack converti detecte.";
    }

    if (!current_pack_id.is_empty()) {
        progress_text += "\nEn jeu: " + current_pack_id;
        if (current_level_index >= 0 && current_level_index < current_pack_levels.size()) {
            const Ref<MnmsLevelResource> level = current_pack_levels[current_level_index];
            if (level.is_valid()) {
                progress_text += " / " + level->get_level_name();
            }
        }
    }
    selector_progress_label->set_text(progress_text);

    String details_text = single_pack_mode ? "Choisis un niveau classique a lancer." : "Choisis un pack puis un niveau.";
    const bool has_selected_level = selected_level_index >= 0 && selected_level_index < selector_pack_levels.size();
    if (has_selected_level) {
        const Ref<MnmsLevelResource> level = selector_pack_levels[selected_level_index];
        if (level.is_valid()) {
            details_text = level->get_level_name();
            details_text += "\nTaille: " + String::num_int64(level->get_world_size().x) + "x" + String::num_int64(level->get_world_size().y);
            details_text += "    Temps cible: " + (level->get_time_limit() >= 0 ? format_ticks_as_time(level->get_time_limit()) : String("-"));
            details_text += "    Enregistrements cible: " + (level->get_recordings_count() >= 0 ? String::num_int64(level->get_recordings_count()) : String("-"));
            const int32_t best_time = _get_best_time_for_level(String(available_pack_ids[selected_pack_index]), selected_level_index);
            const int32_t best_recordings = _get_best_recordings_for_level(String(available_pack_ids[selected_pack_index]), selected_level_index);
            const int32_t medal = _compute_level_medal(best_time, level->get_time_limit(), best_recordings, level->get_recordings_count());
            details_text += "\nMeilleur temps: " + (best_time >= 0 ? format_ticks_as_time(best_time) : String("-"));
            details_text += "    Meilleurs enregistrements: " + (best_recordings >= 0 ? String::num_int64(best_recordings) : String("-"));
            details_text += "    Medaille: ";
            details_text += medal >= 3 ? "OR" : (medal == 2 ? "ARGENT" : (medal == 1 ? "BRONZE" : "-"));
            if (selected_level_index >= selector_unlocked_level_count) {
                details_text += "\nCe niveau est encore verrouille.";
            } else if (current_pack_id == "classic" && _find_current_classic_validation_slot() >= 0 && current_level_index == selected_level_index) {
                details_text += "\nCe niveau est deja charge dans la partie en cours.";
            }
        }
    }
    selector_details_label->set_text(details_text);
    selector_play_button->set_disabled(!has_selected_level || selected_level_index >= selector_unlocked_level_count);
}

void MnmsGameDirector::_ensure_interlevel_ui() {
    if (ui_layer == nullptr) {
        ui_layer = memnew(CanvasLayer);
        ui_layer->set_name("UiLayer");
        add_child(ui_layer);
    }
    if (interlevel_root != nullptr) {
        return;
    }

    interlevel_root = memnew(Control);
    interlevel_root->set_name("InterlevelRoot");
    interlevel_root->set_anchors_preset(Control::PRESET_FULL_RECT);
    interlevel_root->set_visible(false);
    ui_layer->add_child(interlevel_root);

    interlevel_panel = memnew(PanelContainer);
    interlevel_panel->set_name("InterlevelPanel");
    interlevel_panel->set_position(Vector2(172, 96));
    interlevel_panel->set_size(Vector2(456, 394));
    interlevel_root->add_child(interlevel_panel);

    Control *content = memnew(Control);
    content->set_name("InterlevelContent");
    content->set_custom_minimum_size(Vector2(456, 394));
    interlevel_panel->add_child(content);

    interlevel_title_label = memnew(Label);
    interlevel_title_label->set_name("InterlevelTitle");
    interlevel_title_label->set_position(Vector2(24, 24));
    interlevel_title_label->set_size(Vector2(408, 36));
    interlevel_title_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    interlevel_title_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
    content->add_child(interlevel_title_label);

    interlevel_stats_label = memnew(Label);
    interlevel_stats_label->set_name("InterlevelStats");
    interlevel_stats_label->set_position(Vector2(24, 76));
    interlevel_stats_label->set_size(Vector2(408, 136));
    interlevel_stats_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    interlevel_stats_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    interlevel_stats_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
    content->add_child(interlevel_stats_label);

    interlevel_medal_rect = memnew(TextureRect);
    interlevel_medal_rect->set_name("InterlevelMedal");
    interlevel_medal_rect->set_position(Vector2(213, 212));
    interlevel_medal_rect->set_size(Vector2(30, 30));
    interlevel_medal_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
    interlevel_medal_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
    content->add_child(interlevel_medal_rect);

    interlevel_next_button = memnew(Button);
    interlevel_next_button->set_name("InterlevelNextButton");
    interlevel_next_button->set_text("Niveau suivant");
    interlevel_next_button->set_position(Vector2(120, 244));
    interlevel_next_button->set_size(Vector2(216, 34));
    interlevel_next_button->set_focus_mode(Control::FOCUS_ALL);
    interlevel_next_button->connect("pressed", Callable(this, "_on_interlevel_next_pressed"));
    content->add_child(interlevel_next_button);

    interlevel_restart_button = memnew(Button);
    interlevel_restart_button->set_name("InterlevelRestartButton");
    interlevel_restart_button->set_text("Rejouer");
    interlevel_restart_button->set_position(Vector2(120, 286));
    interlevel_restart_button->set_size(Vector2(216, 34));
    interlevel_restart_button->set_focus_mode(Control::FOCUS_ALL);
    interlevel_restart_button->connect("pressed", Callable(this, "_on_interlevel_restart_pressed"));
    content->add_child(interlevel_restart_button);

    interlevel_save_replay_button = memnew(Button);
    interlevel_save_replay_button->set_name("InterlevelSaveReplayButton");
    interlevel_save_replay_button->set_text("Sauvegarder le replay");
    interlevel_save_replay_button->set_position(Vector2(120, 328));
    interlevel_save_replay_button->set_size(Vector2(216, 34));
    interlevel_save_replay_button->set_focus_mode(Control::FOCUS_ALL);
    interlevel_save_replay_button->connect("pressed", Callable(this, "_on_interlevel_save_replay_pressed"));
    content->add_child(interlevel_save_replay_button);

    interlevel_select_button = memnew(Button);
    interlevel_select_button->set_name("InterlevelSelectButton");
    interlevel_select_button->set_text("Retour aux niveaux");
    interlevel_select_button->set_position(Vector2(120, 370));
    interlevel_select_button->set_size(Vector2(216, 34));
    interlevel_select_button->set_focus_mode(Control::FOCUS_ALL);
    interlevel_select_button->connect("pressed", Callable(this, "_on_interlevel_select_pressed"));
    content->add_child(interlevel_select_button);

    interlevel_help_label = memnew(Label);
    interlevel_help_label->set_name("InterlevelHelp");
    interlevel_help_label->set_position(Vector2(24, 228));
    interlevel_help_label->set_size(Vector2(408, 16));
    interlevel_help_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    interlevel_help_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
    content->add_child(interlevel_help_label);

    _refresh_interlevel_ui();
}

void MnmsGameDirector::_refresh_interlevel_ui() {
    if (interlevel_root == nullptr || interlevel_title_label == nullptr || interlevel_stats_label == nullptr) {
        return;
    }

    interlevel_root->set_visible(interlevel_visible);
    if (!interlevel_visible || level_runner == nullptr || level_runner->get_current_level().is_null()) {
        return;
    }

    Ref<MnmsLevelResource> level = level_runner->get_current_level();
    const int32_t elapsed_ticks = level_runner->get_elapsed_ticks();
    const int32_t recordings_used = level_runner->get_recordings_used();
    const int32_t target_time = level_runner->get_target_time();
    const int32_t target_recordings = level_runner->get_target_recordings();
    const bool has_next_level = current_level_index + 1 < current_pack_levels.size();
    const bool beats_time = target_time < 0 || elapsed_ticks <= target_time;
    const bool beats_recordings = target_recordings < 0 || recordings_used <= target_recordings;

    interlevel_title_label->set_text(has_next_level ? "Niveau termine" : "Pack termine");

    String stats_text = level->get_level_name();
    stats_text += "\nTemps: " + format_ticks_as_time(elapsed_ticks);
    if (target_time >= 0) {
        stats_text += " / cible " + format_ticks_as_time(target_time);
        stats_text += beats_time ? "  OK" : "  manquee";
    }
    stats_text += "\nEnregistrements: " + String::num_int64(recordings_used);
    if (target_recordings >= 0) {
        stats_text += " / cible " + String::num_int64(target_recordings);
        stats_text += beats_recordings ? "  OK" : "  manquee";
    }
    if (last_old_best_time >= 0 || last_old_best_recordings >= 0) {
        stats_text += "\nAncien best: ";
        stats_text += last_old_best_time >= 0 ? format_ticks_as_time(last_old_best_time) : String("-");
        stats_text += " / ";
        stats_text += last_old_best_recordings >= 0 ? String::num_int64(last_old_best_recordings) : String("-");
    }
    stats_text += "\nMeilleure medaille: ";
    stats_text += last_completed_medal >= 3 ? "OR" : (last_completed_medal == 2 ? "ARGENT" : (last_completed_medal == 1 ? "BRONZE" : "-"));
    if (last_completed_medal > last_old_best_medal && last_old_best_medal > 0) {
        stats_text += " (amelioree)";
    }
    if (!has_next_level) {
        stats_text += "\nTous les niveaux actuellement debloques dans ce pack sont termines.";
    }
    interlevel_stats_label->set_text(stats_text);
    if (interlevel_medal_rect != nullptr) {
        interlevel_medal_rect->set_texture(build_medal_texture("res://mnms_source_data/gfx/medals.png", last_completed_medal));
        interlevel_medal_rect->set_visible(last_completed_medal > 0);
    }

    interlevel_help_label->set_text("Entree: continuer    Echap: revenir a la selection");
    interlevel_next_button->set_disabled(!has_next_level);
    if (interlevel_save_replay_button != nullptr) {
        interlevel_save_replay_button->set_disabled(!_can_save_replay());
    }
}

void MnmsGameDirector::_ensure_pause_ui() {
    if (ui_layer == nullptr) {
        ui_layer = memnew(CanvasLayer);
        ui_layer->set_name("UiLayer");
        add_child(ui_layer);
    }
    if (pause_root != nullptr) {
        return;
    }

    pause_root = memnew(Control);
    pause_root->set_name("PauseRoot");
    pause_root->set_anchors_preset(Control::PRESET_FULL_RECT);
    pause_root->set_visible(false);
    ui_layer->add_child(pause_root);

    pause_panel = memnew(PanelContainer);
    pause_panel->set_name("PausePanel");
    pause_panel->set_position(Vector2(220, 180));
    pause_panel->set_size(Vector2(360, 456));
    pause_root->add_child(pause_panel);

    Control *content = memnew(Control);
    content->set_name("PauseContent");
    content->set_custom_minimum_size(Vector2(360, 456));
    pause_panel->add_child(content);

    pause_title_label = memnew(Label);
    pause_title_label->set_name("PauseTitle");
    pause_title_label->set_position(Vector2(24, 28));
    pause_title_label->set_size(Vector2(312, 32));
    pause_title_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    pause_title_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
    content->add_child(pause_title_label);

    pause_help_label = memnew(Label);
    pause_help_label->set_name("PauseHelp");
    pause_help_label->set_position(Vector2(24, 72));
    pause_help_label->set_size(Vector2(312, 44));
    pause_help_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    pause_help_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
    pause_help_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    content->add_child(pause_help_label);

    pause_resume_button = memnew(Button);
    pause_resume_button->set_name("PauseResumeButton");
    pause_resume_button->set_text("Reprendre");
    pause_resume_button->set_position(Vector2(72, 124));
    pause_resume_button->set_size(Vector2(216, 34));
    pause_resume_button->set_focus_mode(Control::FOCUS_ALL);
    pause_resume_button->connect("pressed", Callable(this, "_on_pause_resume_pressed"));
    content->add_child(pause_resume_button);

    pause_restart_button = memnew(Button);
    pause_restart_button->set_name("PauseRestartButton");
    pause_restart_button->set_text("Recommencer");
    pause_restart_button->set_position(Vector2(72, 166));
    pause_restart_button->set_size(Vector2(216, 34));
    pause_restart_button->set_focus_mode(Control::FOCUS_ALL);
    pause_restart_button->connect("pressed", Callable(this, "_on_pause_restart_pressed"));
    content->add_child(pause_restart_button);

    pause_checkpoint_button = memnew(Button);
    pause_checkpoint_button->set_name("PauseCheckpointButton");
    pause_checkpoint_button->set_text("Charger le checkpoint");
    pause_checkpoint_button->set_position(Vector2(72, 208));
    pause_checkpoint_button->set_size(Vector2(216, 34));
    pause_checkpoint_button->set_focus_mode(Control::FOCUS_ALL);
    pause_checkpoint_button->connect("pressed", Callable(this, "_on_pause_checkpoint_pressed"));
    content->add_child(pause_checkpoint_button);

    pause_select_button = memnew(Button);
    pause_select_button->set_name("PauseSelectButton");
    pause_select_button->set_text("Choisir un niveau");
    pause_select_button->set_position(Vector2(72, 250));
    pause_select_button->set_size(Vector2(216, 34));
    pause_select_button->set_focus_mode(Control::FOCUS_ALL);
    pause_select_button->connect("pressed", Callable(this, "_on_pause_select_pressed"));
    content->add_child(pause_select_button);

    pause_save_replay_button = memnew(Button);
    pause_save_replay_button->set_name("PauseSaveReplayButton");
    pause_save_replay_button->set_text("Sauvegarder le replay");
    pause_save_replay_button->set_position(Vector2(72, 292));
    pause_save_replay_button->set_size(Vector2(216, 34));
    pause_save_replay_button->set_focus_mode(Control::FOCUS_ALL);
    pause_save_replay_button->connect("pressed", Callable(this, "_on_pause_save_replay_pressed"));
    content->add_child(pause_save_replay_button);

    pause_options_button = memnew(Button);
    pause_options_button->set_name("PauseOptionsButton");
    pause_options_button->set_text("Options");
    pause_options_button->set_position(Vector2(72, 334));
    pause_options_button->set_size(Vector2(216, 34));
    pause_options_button->set_focus_mode(Control::FOCUS_ALL);
    pause_options_button->connect("pressed", Callable(this, "_on_pause_options_pressed"));
    content->add_child(pause_options_button);

    pause_help_button = memnew(Button);
    pause_help_button->set_name("PauseHelpButton");
    pause_help_button->set_text("Aide");
    pause_help_button->set_position(Vector2(72, 376));
    pause_help_button->set_size(Vector2(216, 34));
    pause_help_button->set_focus_mode(Control::FOCUS_ALL);
    pause_help_button->connect("pressed", Callable(this, "_on_pause_help_pressed"));
    content->add_child(pause_help_button);

    pause_stats_button = memnew(Button);
    pause_stats_button->set_name("PauseStatsButton");
    pause_stats_button->set_text("Statistiques");
    pause_stats_button->set_position(Vector2(72, 418));
    pause_stats_button->set_size(Vector2(216, 34));
    pause_stats_button->set_focus_mode(Control::FOCUS_ALL);
    pause_stats_button->connect("pressed", Callable(this, "_on_pause_stats_pressed"));
    content->add_child(pause_stats_button);

    _refresh_pause_ui();
}

void MnmsGameDirector::_refresh_pause_ui() {
    if (pause_root == nullptr || pause_title_label == nullptr || pause_help_label == nullptr) {
        return;
    }

    pause_root->set_visible(pause_visible && modal_screen.is_empty());
    if (!pause_visible) {
        return;
    }

    pause_title_label->set_text("Pause");
    pause_help_label->set_text(
        _get_keycode_label(_get_bound_keycode("mnms_pause")) + " ou P: reprendre    F6/F7: parcours classic\n" +
        _get_keycode_label(_get_bound_keycode("mnms_restart")) + ": recommencer    " +
        _get_keycode_label(_get_bound_keycode("mnms_checkpoint")) + ": checkpoint    " +
        _get_keycode_label(_get_bound_keycode("mnms_level_select")) + ": niveaux    " +
        _get_keycode_label(_get_bound_keycode("mnms_save_replay")) + ": sauvegarder");
    if (pause_checkpoint_button != nullptr) {
        const bool has_checkpoint = level_runner != nullptr && level_runner->has_checkpoint();
        pause_checkpoint_button->set_text(has_checkpoint ? "Charger le checkpoint" : "Recommencer depuis le debut");
    }
    if (pause_save_replay_button != nullptr) {
        pause_save_replay_button->set_disabled(!_can_save_replay());
    }
}

void MnmsGameDirector::_ensure_modal_ui() {
    if (ui_layer == nullptr) {
        ui_layer = memnew(CanvasLayer);
        ui_layer->set_name("UiLayer");
        add_child(ui_layer);
    }
    if (modal_root != nullptr) {
        return;
    }

    modal_root = memnew(Control);
    modal_root->set_name("ModalRoot");
    modal_root->set_anchors_preset(Control::PRESET_FULL_RECT);
    modal_root->set_visible(false);
    ui_layer->add_child(modal_root);

    modal_panel = memnew(PanelContainer);
    modal_panel->set_name("ModalPanel");
    modal_panel->set_position(Vector2(90, 52));
    modal_panel->set_size(Vector2(620, 496));
    modal_root->add_child(modal_panel);

    Control *content = memnew(Control);
    content->set_name("ModalContent");
    content->set_custom_minimum_size(Vector2(620, 496));
    modal_panel->add_child(content);

    modal_title_label = memnew(Label);
    modal_title_label->set_name("ModalTitle");
    modal_title_label->set_position(Vector2(24, 22));
    modal_title_label->set_size(Vector2(572, 28));
    modal_title_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    modal_title_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
    content->add_child(modal_title_label);

    modal_help_label = memnew(Label);
    modal_help_label->set_name("ModalHelp");
    modal_help_label->set_position(Vector2(24, 58));
    modal_help_label->set_size(Vector2(572, 44));
    modal_help_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    modal_help_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    modal_help_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
    content->add_child(modal_help_label);

    modal_list = memnew(ItemList);
    modal_list->set_name("ModalList");
    modal_list->set_position(Vector2(24, 118));
    modal_list->set_size(Vector2(572, 180));
    modal_list->set_focus_mode(Control::FOCUS_ALL);
    modal_list->set_select_mode(ItemList::SELECT_SINGLE);
    modal_list->set_allow_reselect(true);
    modal_list->connect("item_selected", Callable(this, "_on_modal_item_selected"));
    modal_list->connect("item_activated", Callable(this, "_on_modal_item_activated"));
    content->add_child(modal_list);

    modal_body_label = memnew(Label);
    modal_body_label->set_name("ModalBody");
    modal_body_label->set_position(Vector2(24, 310));
    modal_body_label->set_size(Vector2(572, 138));
    modal_body_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    modal_body_label->set_vertical_alignment(VERTICAL_ALIGNMENT_TOP);
    content->add_child(modal_body_label);

    modal_back_button = memnew(Button);
    modal_back_button->set_name("ModalBackButton");
    modal_back_button->set_text("Retour");
    modal_back_button->set_position(Vector2(220, 454));
    modal_back_button->set_size(Vector2(180, 30));
    modal_back_button->set_focus_mode(Control::FOCUS_ALL);
    modal_back_button->connect("pressed", Callable(this, "_on_modal_back_pressed"));
    content->add_child(modal_back_button);

    _refresh_modal_ui();
}

void MnmsGameDirector::_refresh_modal_ui() {
    if (modal_root == nullptr || modal_title_label == nullptr || modal_body_label == nullptr || modal_help_label == nullptr || modal_list == nullptr) {
        return;
    }

    const bool visible = !modal_screen.is_empty();
    modal_root->set_visible(visible);
    if (!visible) {
        return;
    }

    modal_list->clear();
    modal_list->set_visible(false);
    modal_title_label->set_text("");
    modal_help_label->set_text("");
    modal_body_label->set_text("");

    if (modal_screen == "options") {
        modal_title_label->set_text("Options");
        modal_help_label->set_text("Entree: modifier    Echap: retour");
        modal_list->set_visible(true);
        modal_list->add_item(String("Musique: ") + (_get_option_enabled("music_enabled", true) ? "active" : "coupee"));
        modal_list->add_item(String("SFX: ") + (_get_option_enabled("sfx_enabled", true) ? "actifs" : "coupes"));
        modal_list->add_item(String("Notifications: ") + (_get_option_enabled("show_notifications", true) ? "visibles" : "cachees"));
        modal_list->add_item("Configurer les controles");
        modal_list->add_item(recent_replay_files.is_empty() ? "Charger le dernier replay (indisponible)" : "Charger le dernier replay");
        modal_list->add_item("Restaurer les touches par defaut");
        modal_body_label->set_text("Le texte runtime du haut a ete retire du jeu et reste maintenant concentre dans l'ecran Aide.");
    } else if (modal_screen == "controls") {
        modal_title_label->set_text("Controles");
        modal_help_label->set_text(pending_rebind_action.is_empty()
            ? "Entree: reassigner une touche    Echap: retour options"
            : "Appuie sur une touche pour \"" + _get_action_label(pending_rebind_action) + "\". Echap annule.");
        modal_list->set_visible(true);
        for (int32_t i = 0; i < ACTION_BINDING_COUNT; i++) {
            if (!ACTION_BINDINGS[i].configurable) {
                continue;
            }
            const String action_name = ACTION_BINDINGS[i].action;
            modal_list->add_item(_get_action_label(action_name) + ": " + _get_keycode_label(_get_bound_keycode(action_name)));
        }
        modal_list->add_item("Restaurer les touches par defaut");
        modal_list->add_item("Retour aux options");
        modal_body_label->set_text("Les commandes globales F6/F7 restent reservees au parcours golden classic.");
    } else if (modal_screen == "help") {
        modal_title_label->set_text("Aide");
        modal_help_label->set_text("Echap: retour");
        modal_body_label->set_position(Vector2(24, 118));
        modal_body_label->set_size(Vector2(572, 330));
        modal_body_label->set_text(_build_help_text());
    } else if (modal_screen == "credits") {
        modal_title_label->set_text("Credits");
        modal_help_label->set_text("Echap: retour");
        modal_body_label->set_position(Vector2(24, 118));
        modal_body_label->set_size(Vector2(572, 330));
        modal_body_label->set_text(_build_credits_text());
    } else if (modal_screen == "stats") {
        modal_title_label->set_text("Statistiques et achievements");
        modal_help_label->set_text("Echap: retour");
        modal_body_label->set_position(Vector2(24, 118));
        modal_body_label->set_size(Vector2(572, 330));
        modal_body_label->set_text(_build_stats_text());
    }

    if (modal_screen == "options" || modal_screen == "controls") {
        modal_body_label->set_position(Vector2(24, 310));
        modal_body_label->set_size(Vector2(572, 138));
        if (modal_list->get_item_count() > 0 && modal_list->get_selected_items().is_empty()) {
            modal_list->select(0);
        }
        modal_list->grab_focus();
    } else if (modal_back_button != nullptr) {
        modal_back_button->grab_focus();
    }
}

void MnmsGameDirector::_set_modal_screen(const String &p_screen, bool p_from_pause) {
    modal_screen = p_screen;
    modal_from_pause = p_from_pause;
    pending_rebind_action = "";
    _refresh_recent_replays();
    _refresh_modal_ui();
    _refresh_title_ui();
    _refresh_pause_ui();
    _refresh_status_ui();
}

void MnmsGameDirector::_close_modal_screen() {
    const bool return_to_pause = modal_from_pause;
    modal_screen = "";
    modal_from_pause = false;
    pending_rebind_action = "";
    _refresh_modal_ui();
    _refresh_title_ui();
    _refresh_pause_ui();
    _refresh_status_ui();
    if (return_to_pause && pause_resume_button != nullptr) {
        pause_resume_button->grab_focus();
    } else if (title_visible && title_menu_list != nullptr) {
        title_menu_list->grab_focus();
    }
}

void MnmsGameDirector::_set_interlevel_visible(bool p_visible) {
    interlevel_visible = p_visible;
    if (interlevel_visible) {
        title_visible = false;
        selector_visible = false;
        pause_visible = false;
        if (title_root != nullptr) {
            title_root->set_visible(false);
        }
        if (selector_root != nullptr) {
            selector_root->set_visible(false);
        }
        if (pause_root != nullptr) {
            pause_root->set_visible(false);
        }
    }

    _refresh_interlevel_ui();
    _refresh_pause_ui();
    _refresh_title_ui();
    _refresh_selector_ui();
    _refresh_status_ui();
    _refresh_music_state();

    if (interlevel_visible) {
        if (interlevel_next_button != nullptr && !interlevel_next_button->is_disabled()) {
            interlevel_next_button->grab_focus();
        } else if (interlevel_restart_button != nullptr) {
            interlevel_restart_button->grab_focus();
        }
    }
}

void MnmsGameDirector::_set_pause_visible(bool p_visible) {
    if (title_visible || selector_visible || interlevel_visible) {
        p_visible = false;
    }

    if (!p_visible && modal_from_pause && !modal_screen.is_empty()) {
        _close_modal_screen();
    }

    pause_visible = p_visible;
    SceneTree *tree = get_tree();
    if (tree != nullptr) {
        tree->set_pause(pause_visible);
    }
    if (music_player != nullptr) {
        music_player->set_stream_paused(pause_visible);
    }

    _refresh_pause_ui();
    _refresh_status_ui();

    if (pause_visible && pause_resume_button != nullptr) {
        pause_resume_button->grab_focus();
    }
}

bool MnmsGameDirector::_can_save_replay() const {
    return replay_system != nullptr &&
        replay_system->get_frame_count() > 0 &&
        (player_shadow_system == nullptr || !player_shadow_system->is_recording());
}

bool MnmsGameDirector::_save_current_replay() {
    if (!_can_save_replay()) {
        UtilityFunctions::print("MNMS: aucun replay a sauvegarder.");
        return false;
    }

    if (DirAccess::make_dir_recursive_absolute(replay_directory_path) != OK) {
        UtilityFunctions::push_warning("MNMS: impossible de creer le dossier replay: " + replay_directory_path);
        return false;
    }

    Dictionary metadata;
    metadata["pack_id"] = current_pack_id;
    metadata["level_index"] = current_level_index;
    metadata["elapsed_ticks"] = last_completed_time >= 0 ? last_completed_time : (level_runner != nullptr ? level_runner->get_elapsed_ticks() : -1);
    metadata["recordings_used"] = last_completed_recordings >= 0 ? last_completed_recordings : (level_runner != nullptr ? level_runner->get_recordings_used() : -1);
    metadata["medal"] = last_completed_medal;
    if (level_runner != nullptr && !level_runner->get_current_level().is_null()) {
        metadata["level_name"] = level_runner->get_current_level()->get_level_name();
    }

    const String replay_path = _build_replay_file_path();
    const bool saved = replay_system->save_to_file(replay_path, metadata);
    if (saved) {
        last_saved_replay_path = replay_path;
        _add_stat("replay_saves");
        _refresh_recent_replays();
        _save_progress();
        UtilityFunctions::print("MNMS: replay sauvegarde -> ", replay_path);
        _show_runtime_message("Replay sauvegarde: " + replay_path.get_file());
        if (interlevel_help_label != nullptr && interlevel_visible) {
            interlevel_help_label->set_text("Replay sauvegarde: " + replay_path.get_file() + "    Entree: continuer    Echap: revenir a la selection");
        }
    } else {
        UtilityFunctions::push_warning(String("MNMS: echec sauvegarde replay -> ") + replay_path);
    }
    return saved;
}

String MnmsGameDirector::_build_replay_file_path() const {
    String level_token = String("level_") + String::num_int64(MAX(0, current_level_index + 1));
    if (level_runner != nullptr && !level_runner->get_current_level().is_null()) {
        level_token += String("_") + sanitize_replay_token(level_runner->get_current_level()->get_level_name());
    }

    const String pack_token = sanitize_replay_token(current_pack_id.is_empty() ? String("classic") : current_pack_id);
    const String base_name = pack_token + String("_") + level_token;
    String replay_path = replay_directory_path.path_join(base_name + String(".cfg"));
    if (!FileAccess::file_exists(replay_path)) {
        return replay_path;
    }

    for (int32_t i = 2; i < 1000; i++) {
        replay_path = replay_directory_path.path_join(base_name + String("_") + String::num_int64(i) + String(".cfg"));
        if (!FileAccess::file_exists(replay_path)) {
            return replay_path;
        }
    }

    return replay_directory_path.path_join(base_name + String("_overflow.cfg"));
}

bool MnmsGameDirector::_load_music_library() {
    menu_music_intro_path = "";
    menu_music_loop_path = "";
    gameplay_music_paths.clear();

    const Dictionary menu_definition = parse_music_definition(source_data_path.path_join("music").path_join("menu.music"));
    if (menu_definition.has("file")) {
        menu_music_intro_path = menu_definition["file"];
    }
    if (menu_definition.has("loopfile")) {
        menu_music_loop_path = menu_definition["loopfile"];
    }

    String music_list_path = source_data_path.path_join("music").path_join("default.list");
    if (!current_pack_music_list.is_empty()) {
        const String candidate_path = source_data_path.path_join("music").path_join(current_pack_music_list);
        if (FileAccess::file_exists(candidate_path)) {
            music_list_path = candidate_path;
        } else {
            UtilityFunctions::push_warning("MNMS: music list introuvable pour le pack, fallback default -> " + candidate_path);
        }
    }

    gameplay_music_paths = parse_music_list_file(music_list_path);
    gameplay_music_index = 0;
    return !menu_music_intro_path.is_empty() || !gameplay_music_paths.is_empty();
}

void MnmsGameDirector::_play_music_stream(const String &p_path, bool p_is_menu_loop) {
    if (music_player == nullptr || p_path.is_empty()) {
        return;
    }
    if (active_music_stream_path == p_path && active_music_is_menu_loop == p_is_menu_loop && music_player->is_playing()) {
        return;
    }

    Ref<AudioStream> stream;
    if (audio_stream_cache.has(p_path)) {
        stream = audio_stream_cache[p_path];
    } else {
        stream = ResourceLoader::get_singleton()->load(p_path);
        if (stream.is_null()) {
            UtilityFunctions::printerr("MNMS: musique introuvable: ", p_path);
            return;
        }
        audio_stream_cache[p_path] = stream;
    }

    active_music_stream_path = p_path;
    active_music_is_menu_loop = p_is_menu_loop;
    music_player->stop();
    music_player->set_stream(stream);
    music_player->set_stream_paused(false);
    music_player->play();
}

void MnmsGameDirector::_refresh_music_state() {
    if (music_player == nullptr) {
        return;
    }

    if (title_visible || selector_visible) {
        current_music_mode = "menu";
        if (!menu_music_intro_path.is_empty() &&
            active_music_stream_path != menu_music_intro_path &&
            !(active_music_is_menu_loop && active_music_stream_path == menu_music_loop_path)) {
            _play_music_stream(menu_music_intro_path, false);
        }
        return;
    }

    if (!gameplay_music_paths.is_empty()) {
        current_music_mode = "gameplay";
        gameplay_music_index = CLAMP(gameplay_music_index, 0, gameplay_music_paths.size() - 1);
        const String music_path = gameplay_music_paths[gameplay_music_index];
        if (active_music_stream_path != music_path || active_music_is_menu_loop) {
            _play_music_stream(music_path, false);
        }
    }
}

void MnmsGameDirector::_play_sfx(const String &p_relative_path) {
    if (sfx_player == nullptr || p_relative_path.is_empty()) {
        return;
    }

    Ref<AudioStream> stream;
    if (audio_stream_cache.has(p_relative_path)) {
        stream = audio_stream_cache[p_relative_path];
    } else {
        const String full_path = source_data_path.path_join("sfx").path_join(p_relative_path);
        stream = ResourceLoader::get_singleton()->load(full_path);
        if (stream.is_null()) {
            UtilityFunctions::printerr("MNMS: sfx introuvable: ", full_path);
            return;
        }
        audio_stream_cache[p_relative_path] = stream;
    }

    sfx_player->set_stream(stream);
    sfx_player->play();
}

void MnmsGameDirector::_refresh_status_ui(int32_t p_elapsed_ticks, int32_t p_recordings_used, int32_t p_target_time, int32_t p_target_recordings) {
    if (status_label == nullptr) {
        return;
    }
    status_label->set_text("");
    status_label->set_visible(false);
}

void MnmsGameDirector::_set_title_visible(bool p_visible) {
    title_visible = p_visible;
    if (title_root != nullptr) {
        title_root->set_visible(title_visible);
    }
    if (title_visible) {
        interlevel_visible = false;
        pause_visible = false;
        if (interlevel_root != nullptr) {
            interlevel_root->set_visible(false);
        }
        if (pause_root != nullptr) {
            pause_root->set_visible(false);
        }
        selector_visible = false;
        if (selector_root != nullptr) {
            selector_root->set_visible(false);
        }
        if (title_menu_list != nullptr) {
            if (title_menu_list->get_item_count() > 0 && title_menu_list->get_selected_items().is_empty()) {
                title_menu_list->select(0);
            }
            title_menu_list->grab_focus();
        }
    }
    _refresh_title_ui();
    _refresh_pause_ui();
    _refresh_selector_ui();
    _refresh_status_ui();
    _refresh_music_state();
}

void MnmsGameDirector::_activate_title_selection() {
    if (title_menu_list == nullptr) {
        return;
    }

    PackedInt32Array selected_items = title_menu_list->get_selected_items();
    const int32_t selected_index = selected_items.is_empty() ? 0 : selected_items[0];
    const String selected_text = title_menu_list->get_item_text(selected_index);
    if (selected_text == "Reprendre") {
        if (_load_first_level()) {
            _set_title_visible(false);
            _set_selector_visible(false);
        }
    } else if (selected_text == "Choisir un niveau") {
        _set_title_visible(false);
        _set_selector_visible(true);
    } else if (selected_text == "Options") {
        _set_modal_screen("options");
    } else if (selected_text == "Aide") {
        _set_modal_screen("help");
    } else if (selected_text == "Statistiques") {
        _set_modal_screen("stats");
    } else if (selected_text == "Credits") {
        _set_modal_screen("credits");
    } else if (selected_text == "Relire le dernier replay") {
        if (!recent_replay_files.is_empty() && _load_replay_from_file(String(recent_replay_files[0]))) {
            _set_title_visible(false);
            _set_selector_visible(false);
        }
    } else if (selected_text == "Quitter") {
        SceneTree *tree = get_tree();
        if (tree != nullptr) {
            tree->quit();
        }
    }
}

void MnmsGameDirector::_toggle_selector() {
    _set_selector_visible(!selector_visible);
}

void MnmsGameDirector::_set_selector_visible(bool p_visible) {
    if (title_visible && p_visible) {
        title_visible = false;
        if (title_root != nullptr) {
            title_root->set_visible(false);
        }
    }
    if (interlevel_visible && p_visible) {
        interlevel_visible = false;
        if (interlevel_root != nullptr) {
            interlevel_root->set_visible(false);
        }
    }
    if (pause_visible && p_visible) {
        _set_pause_visible(false);
    }
    selector_visible = p_visible;
    if (selector_visible && !available_pack_ids.is_empty()) {
        selected_pack_index = CLAMP(selected_pack_index, 0, available_pack_ids.size() - 1);
        _load_selector_pack(available_pack_ids[selected_pack_index]);
        _focus_selector_control();
    }
    _refresh_selector_ui();
    _refresh_status_ui();
    _refresh_music_state();
}

void MnmsGameDirector::_wire_runtime_systems() {
    if (player_shadow_system == nullptr || replay_system == nullptr || level_runner == nullptr) {
        return;
    }

    MnmsPlayer *player = level_runner->get_player();
    MnmsShadow *shadow = level_runner->get_shadow();
    if (player == nullptr || shadow == nullptr) {
        return;
    }

    player_shadow_system->bind_runtime(player->get_path(), shadow->get_path(), replay_system->get_path());
    _refresh_status_ui();
}

void MnmsGameDirector::_restart_current_level() {
    if (level_runner == nullptr || current_level_index < 0) {
        return;
    }

    _set_interlevel_visible(false);
    _add_stat("restarts");
    level_runner->restart_level();
    _wire_runtime_systems();
    _show_runtime_message("Niveau recommence.");
    _refresh_status_ui();
}

void MnmsGameDirector::_load_checkpoint_or_restart() {
    if (level_runner == nullptr) {
        return;
    }

    if (level_runner->has_checkpoint()) {
        level_runner->load_checkpoint();
        player_shadow_system->restore_checkpoint_state();
        _add_stat("checkpoint_loads");
        _show_runtime_message("Checkpoint charge.");
    } else {
        _restart_current_level();
    }
    _refresh_status_ui();
}

void MnmsGameDirector::_advance_to_next_level() {
    const int32_t next_level_index = current_level_index + 1;
    current_unlocked_level_count = MAX(current_unlocked_level_count, next_level_index + 1);
    _set_unlocked_level_count(current_pack_id, current_unlocked_level_count);
    _save_progress();

    if (!_load_level_at_index(next_level_index)) {
        _set_interlevel_visible(false);
        _set_selector_visible(true);
        UtilityFunctions::print("MNMS: fin du pack atteint.");
        return;
    }
    _set_interlevel_visible(false);
}

void MnmsGameDirector::_focus_selector_control() {
    if (selector_level_list != nullptr && selector_level_list->is_inside_tree()) {
        selector_level_list->grab_focus();
        return;
    }
    if (selector_pack_list != nullptr && selector_pack_list->is_inside_tree()) {
        selector_pack_list->grab_focus();
    }
}

void MnmsGameDirector::_move_selector_focus(int32_t p_direction) {
    if (!selector_visible || p_direction == 0) {
        return;
    }

    const bool single_pack_mode = available_pack_ids.size() <= 1;
    Control *focused = Object::cast_to<Control>(get_viewport()->gui_get_focus_owner());
    if (single_pack_mode) {
        if (focused == selector_level_list && p_direction > 0) {
            selector_play_button->grab_focus();
        } else if ((focused == selector_play_button || focused == selector_back_button) && p_direction < 0) {
            selector_level_list->grab_focus();
        } else {
            _focus_selector_control();
        }
        return;
    }

    if (focused == selector_pack_list && p_direction > 0) {
        selector_level_list->grab_focus();
    } else if (focused == selector_level_list) {
        if (p_direction > 0) {
            selector_play_button->grab_focus();
        } else {
            selector_pack_list->grab_focus();
        }
    } else if (focused == selector_play_button && p_direction < 0) {
        selector_level_list->grab_focus();
    } else {
        _focus_selector_control();
    }
}

void MnmsGameDirector::_on_selector_pack_selected(int32_t p_index) {
    if (p_index < 0 || p_index >= available_pack_ids.size()) {
        return;
    }

    selected_pack_index = p_index;
    selected_level_index = 0;
    _load_selector_pack(available_pack_ids[selected_pack_index]);
    _refresh_selector_ui();
}

void MnmsGameDirector::_on_selector_pack_activated(int32_t p_index) {
    _on_selector_pack_selected(p_index);
    if (selector_level_list != nullptr) {
        selector_level_list->grab_focus();
    }
}

void MnmsGameDirector::_on_selector_level_selected(int32_t p_index) {
    if (p_index < 0 || p_index >= selector_pack_levels.size()) {
        return;
    }

    selected_level_index = p_index;
    _refresh_selector_ui();
}

void MnmsGameDirector::_on_selector_level_activated(int32_t p_index) {
    _on_selector_level_selected(p_index);
    _on_selector_play_pressed();
}

void MnmsGameDirector::_on_selector_play_pressed() {
    if (_load_selected_level()) {
        _set_interlevel_visible(false);
        _set_selector_visible(false);
    } else {
        _refresh_selector_ui();
    }
}

void MnmsGameDirector::_process(double delta) {
    (void)delta;

    if (player_shadow_system == nullptr) {
        return;
    }

    Input *input = Input::get_singleton();
    const bool restart_now = input->is_action_pressed("mnms_restart");
    const bool load_checkpoint_now = input->is_action_pressed("mnms_checkpoint");
    const bool record_now = input->is_action_pressed("mnms_record_toggle");
    const bool cancel_record_now = input->is_action_pressed("mnms_cancel_or_stop");
    const bool save_replay_now = input->is_action_pressed("mnms_save_replay");
    const bool shadow_view_now = input->is_action_pressed("mnms_shadow_view");
    const bool replay_restart_now = input->is_action_pressed("mnms_replay_restart");
    const bool replay_stop_now = input->is_action_pressed("mnms_cancel_or_stop");
    const bool pause_now = input->is_action_pressed("mnms_pause");
    const bool selector_toggle_now = input->is_action_pressed("mnms_level_select");
    const bool validation_prev_now = input->is_action_pressed("mnms_validation_prev");
    const bool validation_next_now = input->is_action_pressed("mnms_validation_next");
    const bool selector_left_now = input->is_action_pressed("ui_left");
    const bool selector_right_now = input->is_action_pressed("ui_right");
    const bool selector_cancel_now = input->is_action_pressed("ui_cancel");
    const bool title_up_now = input->is_action_pressed("ui_up");
    const bool title_down_now = input->is_action_pressed("ui_down");
    const bool title_select_now = input->is_action_pressed("ui_accept");
    const bool title_cancel_now = input->is_action_pressed("ui_cancel");

    if (!title_visible) {
        play_time_accumulator += delta;
        stats_save_accumulator += delta;
        while (play_time_accumulator >= 1.0) {
            _set_stat_value("play_time_seconds", _get_stat_value("play_time_seconds") + 1);
            play_time_accumulator -= 1.0;
        }
        if (stats_save_accumulator >= 5.0) {
            _save_progress();
            stats_save_accumulator = 0.0;
        }
    }

    bool validation_jump = false;
    if (validation_prev_now && !validation_prev_key_down) {
        validation_jump = _jump_to_classic_validation_level(-1);
    } else if (validation_next_now && !validation_next_key_down) {
        validation_jump = _jump_to_classic_validation_level(1);
    }
    if (validation_jump) {
        restart_key_down = restart_now;
        load_checkpoint_key_down = load_checkpoint_now;
        record_key_down = record_now;
        cancel_record_key_down = cancel_record_now;
        save_replay_key_down = save_replay_now;
        shadow_view_key_down = shadow_view_now;
        replay_restart_key_down = replay_restart_now;
        replay_stop_key_down = replay_stop_now;
        pause_key_down = pause_now;
        validation_prev_key_down = validation_prev_now;
        validation_next_key_down = validation_next_now;
        title_up_key_down = title_up_now;
        title_down_key_down = title_down_now;
        title_select_key_down = title_select_now;
        title_cancel_key_down = title_cancel_now;
        selector_toggle_key_down = selector_toggle_now;
        selector_left_key_down = selector_left_now;
        selector_right_key_down = selector_right_now;
        selector_cancel_key_down = selector_cancel_now;
        return;
    }

    if (!modal_screen.is_empty()) {
        if (title_cancel_now && !title_cancel_key_down) {
            _on_modal_back_pressed();
        } else if (title_select_now && !title_select_key_down && modal_list != nullptr && modal_list->has_focus()) {
            PackedInt32Array selected_items = modal_list->get_selected_items();
            if (!selected_items.is_empty()) {
                _on_modal_item_activated(selected_items[0]);
            }
        }

        restart_key_down = restart_now;
        load_checkpoint_key_down = load_checkpoint_now;
        record_key_down = record_now;
        cancel_record_key_down = cancel_record_now;
        save_replay_key_down = save_replay_now;
        shadow_view_key_down = shadow_view_now;
        replay_restart_key_down = replay_restart_now;
        replay_stop_key_down = replay_stop_now;
        pause_key_down = pause_now;
        validation_prev_key_down = validation_prev_now;
        validation_next_key_down = validation_next_now;
        title_up_key_down = title_up_now;
        title_down_key_down = title_down_now;
        title_select_key_down = title_select_now;
        title_cancel_key_down = title_cancel_now;
        selector_toggle_key_down = selector_toggle_now;
        selector_left_key_down = selector_left_now;
        selector_right_key_down = selector_right_now;
        selector_cancel_key_down = selector_cancel_now;
        return;
    }

    if (title_visible) {
        if (title_up_now && !title_up_key_down && title_menu_list != nullptr) {
            PackedInt32Array selected_items = title_menu_list->get_selected_items();
            int32_t index = selected_items.is_empty() ? 0 : selected_items[0];
            index = (index - 1 + title_menu_list->get_item_count()) % MAX(1, title_menu_list->get_item_count());
            title_menu_list->select(index);
        }
        if (title_down_now && !title_down_key_down && title_menu_list != nullptr) {
            PackedInt32Array selected_items = title_menu_list->get_selected_items();
            int32_t index = selected_items.is_empty() ? 0 : selected_items[0];
            index = (index + 1) % MAX(1, title_menu_list->get_item_count());
            title_menu_list->select(index);
        }
        if (title_select_now && !title_select_key_down) {
            _activate_title_selection();
        }
        if (title_cancel_now && !title_cancel_key_down) {
            SceneTree *tree = get_tree();
            if (tree != nullptr) {
                tree->quit();
            }
        }

        title_up_key_down = title_up_now;
        title_down_key_down = title_down_now;
        title_select_key_down = title_select_now;
        title_cancel_key_down = title_cancel_now;
        selector_toggle_key_down = selector_toggle_now;
        validation_prev_key_down = validation_prev_now;
        validation_next_key_down = validation_next_now;
        selector_left_key_down = selector_left_now;
        selector_right_key_down = selector_right_now;
        selector_cancel_key_down = selector_cancel_now;
        cancel_record_key_down = cancel_record_now;
        save_replay_key_down = save_replay_now;
        replay_restart_key_down = replay_restart_now;
        replay_stop_key_down = replay_stop_now;
        pause_key_down = pause_now;
        return;
    }

    if (selector_toggle_now && !selector_toggle_key_down) {
        _toggle_selector();
    }

    if (selector_visible) {
        if (selector_cancel_now && !selector_cancel_key_down) {
            if (level_runner == nullptr || level_runner->get_current_level().is_null()) {
                _set_title_visible(true);
            } else {
                _set_selector_visible(false);
            }
        }
        if (selector_left_now && !selector_left_key_down) {
            _move_selector_focus(-1);
        }
        if (selector_right_now && !selector_right_key_down) {
            _move_selector_focus(1);
        }

        selector_toggle_key_down = selector_toggle_now;
        validation_prev_key_down = validation_prev_now;
        validation_next_key_down = validation_next_now;
        selector_left_key_down = selector_left_now;
        selector_right_key_down = selector_right_now;
        selector_cancel_key_down = selector_cancel_now;
        cancel_record_key_down = cancel_record_now;
        save_replay_key_down = save_replay_now;
        replay_restart_key_down = replay_restart_now;
        replay_stop_key_down = replay_stop_now;
        pause_key_down = pause_now;
        return;
    }

    if (interlevel_visible) {
        if (selector_cancel_now && !selector_cancel_key_down) {
            _on_interlevel_select_pressed();
        }
        if (save_replay_now && !save_replay_key_down) {
            _save_current_replay();
        }
        selector_cancel_key_down = selector_cancel_now;
        selector_toggle_key_down = selector_toggle_now;
        validation_prev_key_down = validation_prev_now;
        validation_next_key_down = validation_next_now;
        selector_left_key_down = selector_left_now;
        selector_right_key_down = selector_right_now;
        restart_key_down = restart_now;
        load_checkpoint_key_down = load_checkpoint_now;
        record_key_down = record_now;
        cancel_record_key_down = cancel_record_now;
        save_replay_key_down = save_replay_now;
        shadow_view_key_down = shadow_view_now;
        replay_restart_key_down = replay_restart_now;
        replay_stop_key_down = replay_stop_now;
        pause_key_down = pause_now;
        return;
    }

    if (pause_now && !pause_key_down) {
        _set_pause_visible(!pause_visible);
    }
    if (pause_visible) {
        if (restart_now && !restart_key_down) {
            _set_pause_visible(false);
            player_shadow_system->stop_replay();
            _restart_current_level();
        }
        if (load_checkpoint_now && !load_checkpoint_key_down) {
            _set_pause_visible(false);
            player_shadow_system->stop_replay();
            _load_checkpoint_or_restart();
        }
        if (selector_toggle_now && !selector_toggle_key_down) {
            _set_pause_visible(false);
            _set_selector_visible(true);
        }
        if (save_replay_now && !save_replay_key_down) {
            _save_current_replay();
        }
        if (replay_restart_now && !replay_restart_key_down) {
            if (player_shadow_system->restart_replay()) {
                _play_sfx("toggle.ogg");
                _refresh_status_ui();
            }
        }
        if (replay_stop_now && !replay_stop_key_down) {
            if (player_shadow_system->stop_replay_playback()) {
                _play_sfx("error.wav");
                _refresh_status_ui();
            }
        }

        restart_key_down = restart_now;
        load_checkpoint_key_down = load_checkpoint_now;
        record_key_down = record_now;
        cancel_record_key_down = cancel_record_now;
        save_replay_key_down = save_replay_now;
        shadow_view_key_down = shadow_view_now;
        replay_restart_key_down = replay_restart_now;
        replay_stop_key_down = replay_stop_now;
        pause_key_down = pause_now;
        validation_prev_key_down = validation_prev_now;
        validation_next_key_down = validation_next_now;
        title_up_key_down = title_up_now;
        title_down_key_down = title_down_now;
        title_select_key_down = title_select_now;
        title_cancel_key_down = title_cancel_now;
        selector_toggle_key_down = selector_toggle_now;
        selector_left_key_down = selector_left_now;
        selector_right_key_down = selector_right_now;
        selector_cancel_key_down = selector_cancel_now;
        return;
    }

    const bool recording_started = player_shadow_system->consume_recording_started();
    const bool recording_cancelled = player_shadow_system->consume_recording_cancelled();
    const bool replay_started = player_shadow_system->consume_replay_started();
    if (recording_started && level_runner != nullptr) {
        level_runner->increment_recordings_used();
        _add_stat("recordings_started");
    }
    if (recording_cancelled && level_runner != nullptr) {
        level_runner->set_recordings_used(MAX(0, level_runner->get_recordings_used() - 1));
    }
    if (recording_started) {
        _play_sfx("toggle.ogg");
    }
    if (recording_cancelled) {
        _play_sfx("error.wav");
    }
    if (replay_started) {
        _play_sfx("toggle.ogg");
    }
    if (recording_started || recording_cancelled || replay_started) {
        _refresh_status_ui();
    }

    if (restart_now && !restart_key_down) {
        player_shadow_system->stop_replay();
        _restart_current_level();
    }
    if (load_checkpoint_now && !load_checkpoint_key_down) {
        player_shadow_system->stop_replay();
        _load_checkpoint_or_restart();
    }
    if (shadow_view_now && !shadow_view_key_down) {
        player_shadow_system->toggle_shadow_view();
        _refresh_status_ui();
    }
    if (record_now && !record_key_down) {
        player_shadow_system->request_record_toggle();
    }
    if (cancel_record_now && !cancel_record_key_down) {
        if (player_shadow_system->is_replaying()) {
            if (player_shadow_system->stop_replay_playback()) {
                _play_sfx("error.wav");
                _refresh_status_ui();
            }
        } else {
            player_shadow_system->request_record_cancel();
        }
    }
    if (save_replay_now && !save_replay_key_down) {
        _save_current_replay();
    }
    if (replay_restart_now && !replay_restart_key_down) {
        if (player_shadow_system->restart_replay()) {
            _play_sfx("toggle.ogg");
            _refresh_status_ui();
        }
    }
    if (replay_stop_now && !replay_stop_key_down) {
        if (player_shadow_system->stop_replay_playback()) {
            _play_sfx("error.wav");
            _refresh_status_ui();
        }
    }

    restart_key_down = restart_now;
    load_checkpoint_key_down = load_checkpoint_now;
    record_key_down = record_now;
    cancel_record_key_down = cancel_record_now;
    save_replay_key_down = save_replay_now;
    shadow_view_key_down = shadow_view_now;
    replay_restart_key_down = replay_restart_now;
    replay_stop_key_down = replay_stop_now;
    pause_key_down = pause_now;
    validation_prev_key_down = validation_prev_now;
    validation_next_key_down = validation_next_now;
    title_up_key_down = title_up_now;
    title_down_key_down = title_down_now;
    title_select_key_down = title_select_now;
    title_cancel_key_down = title_cancel_now;
    selector_toggle_key_down = selector_toggle_now;
    selector_left_key_down = selector_left_now;
    selector_right_key_down = selector_right_now;
    selector_cancel_key_down = selector_cancel_now;
}

void MnmsGameDirector::_input(const Ref<InputEvent> &event) {
    if (pending_rebind_action.is_empty() || modal_screen != "controls" || event.is_null()) {
        return;
    }

    Ref<InputEventKey> key_event = event;
    if (key_event.is_null() || !key_event->is_pressed() || key_event->is_echo()) {
        return;
    }

    const int32_t keycode = (int32_t)key_event->get_keycode();
    if (keycode == KEY_ESCAPE) {
        pending_rebind_action = "";
        _refresh_modal_ui();
        return;
    }
    if (keycode == KEY_NONE) {
        return;
    }

    _apply_control_binding(pending_rebind_action, keycode);
    pending_rebind_action = "";
    _save_progress();
    _show_runtime_message("Commande mise a jour.");
    _refresh_modal_ui();
    _refresh_status_ui();
}

void MnmsGameDirector::_on_level_failed() {
    UtilityFunctions::print("MNMS: echec, retour checkpoint ou restart.");
    _play_sfx("hit.wav");
    _add_stat("deaths");
    player_shadow_system->stop_replay();
    _load_checkpoint_or_restart();
}

void MnmsGameDirector::_on_level_completed(const String &p_level_name) {
    UtilityFunctions::print("MNMS: niveau termine -> ", p_level_name);
    player_shadow_system->stop_replay();
    const int32_t elapsed_ticks = level_runner != nullptr ? level_runner->get_elapsed_ticks() : -1;
    const int32_t recordings_used = level_runner != nullptr ? level_runner->get_recordings_used() : -1;
    const int32_t target_time = level_runner != nullptr ? level_runner->get_target_time() : -1;
    const int32_t target_recordings = level_runner != nullptr ? level_runner->get_target_recordings() : -1;
    last_completed_time = elapsed_ticks;
    last_completed_recordings = recordings_used;
    last_old_best_time = _get_best_time_for_level(current_pack_id, current_level_index);
    last_old_best_recordings = _get_best_recordings_for_level(current_pack_id, current_level_index);
    last_old_best_medal = _compute_level_medal(last_old_best_time, target_time, last_old_best_recordings, target_recordings);

    const String record_key = _get_level_record_key(current_pack_id, current_level_index);
    if (elapsed_ticks >= 0 && (last_old_best_time < 0 || elapsed_ticks < last_old_best_time)) {
        best_times_by_level[record_key] = elapsed_ticks;
    }
    if (recordings_used >= 0 && (last_old_best_recordings < 0 || recordings_used < last_old_best_recordings)) {
        best_recordings_by_level[record_key] = recordings_used;
    }
    last_completed_medal = _compute_level_medal(
        _get_best_time_for_level(current_pack_id, current_level_index),
        target_time,
        _get_best_recordings_for_level(current_pack_id, current_level_index),
        target_recordings);
    _set_stat_value("levels_completed", best_times_by_level.keys().size());
    int32_t gold_medals = 0;
    Array record_keys = best_times_by_level.keys();
    for (int32_t i = 0; i < record_keys.size(); i++) {
        const String key = record_keys[i];
        const PackedStringArray parts = key.split(":");
        if (parts.size() != 2) {
            continue;
        }
        const String pack_id = parts[0];
        const int32_t level_index = parts[1].to_int();
        const String pack_path = converted_root_path.path_join("packs").path_join(pack_id + String("_pack.tres"));
        Ref<MnmsPackResource> pack = ResourceLoader::get_singleton()->load(pack_path);
        if (pack.is_null()) {
            continue;
        }
        Array pack_levels = pack->get_levels();
        if (level_index < 0 || level_index >= pack_levels.size()) {
            continue;
        }
        const Ref<MnmsLevelResource> level = pack_levels[level_index];
        if (!level.is_valid()) {
            continue;
        }
        if (_compute_level_medal(
                _get_best_time_for_level(pack_id, level_index),
                level->get_time_limit(),
                _get_best_recordings_for_level(pack_id, level_index),
                level->get_recordings_count()) >= 3) {
            gold_medals += 1;
        }
    }
    _set_stat_value("gold_medals", gold_medals);
    _refresh_achievements();
    const int32_t next_level_index = current_level_index + 1;
    current_unlocked_level_count = MAX(current_unlocked_level_count, next_level_index + 1);
    _set_unlocked_level_count(current_pack_id, current_unlocked_level_count);
    _save_progress();
    _refresh_selector_ui();
    _play_sfx("achievement.ogg");
    _set_interlevel_visible(true);
}

void MnmsGameDirector::_on_title_item_selected(int32_t p_index) {
    if (title_menu_list == nullptr || p_index < 0 || p_index >= title_menu_list->get_item_count()) {
        return;
    }
    title_menu_list->select(p_index);
}

void MnmsGameDirector::_on_title_item_activated(int32_t p_index) {
    _on_title_item_selected(p_index);
    _activate_title_selection();
}

void MnmsGameDirector::_on_checkpoint_activated() {
    if (player_shadow_system != nullptr) {
        player_shadow_system->capture_checkpoint_state();
    }
    _add_stat("checkpoint_activations");
    _play_sfx("checkpoint.wav");
}

void MnmsGameDirector::_on_collectable_collected() {
    _add_stat("collectables");
    _play_sfx("collect.wav");
}

void MnmsGameDirector::_on_swap_activated() {
    _add_stat("swaps");
    _play_sfx("swap.wav");
}

void MnmsGameDirector::_on_runtime_state_changed(int32_t p_elapsed_ticks, int32_t p_recordings_used, int32_t p_target_time, int32_t p_target_recordings) {
    _refresh_status_ui(p_elapsed_ticks, p_recordings_used, p_target_time, p_target_recordings);
}

void MnmsGameDirector::_on_notification_changed(const String &p_message) {
    if (notification_label == nullptr) {
        return;
    }
    notification_label->set_text(p_message);
    notification_label->set_visible(!p_message.is_empty() && _get_option_enabled("show_notifications", true) && modal_screen.is_empty());
}

void MnmsGameDirector::_on_interlevel_next_pressed() {
    _advance_to_next_level();
}

void MnmsGameDirector::_on_interlevel_restart_pressed() {
    _restart_current_level();
}

void MnmsGameDirector::_on_interlevel_save_replay_pressed() {
    _save_current_replay();
}

void MnmsGameDirector::_on_interlevel_select_pressed() {
    _set_interlevel_visible(false);
    _set_selector_visible(true);
}

void MnmsGameDirector::_on_pause_resume_pressed() {
    _set_pause_visible(false);
}

void MnmsGameDirector::_on_pause_restart_pressed() {
    if (player_shadow_system != nullptr) {
        player_shadow_system->stop_replay();
    }
    _set_pause_visible(false);
    _restart_current_level();
}

void MnmsGameDirector::_on_pause_checkpoint_pressed() {
    if (player_shadow_system != nullptr) {
        player_shadow_system->stop_replay();
    }
    _set_pause_visible(false);
    _load_checkpoint_or_restart();
}

void MnmsGameDirector::_on_pause_select_pressed() {
    _set_pause_visible(false);
    _set_selector_visible(true);
}

void MnmsGameDirector::_on_pause_save_replay_pressed() {
    _save_current_replay();
    _refresh_pause_ui();
    _refresh_status_ui();
}

void MnmsGameDirector::_on_pause_options_pressed() {
    _set_modal_screen("options", true);
}

void MnmsGameDirector::_on_pause_help_pressed() {
    _set_modal_screen("help", true);
}

void MnmsGameDirector::_on_pause_stats_pressed() {
    _set_modal_screen("stats", true);
}

void MnmsGameDirector::_on_modal_item_selected(int32_t p_index) {
    if (modal_list == nullptr || p_index < 0 || p_index >= modal_list->get_item_count()) {
        return;
    }
    modal_list->select(p_index);
}

void MnmsGameDirector::_on_modal_item_activated(int32_t p_index) {
    _on_modal_item_selected(p_index);

    if (modal_screen == "options") {
        switch (p_index) {
        case 0:
            _set_option_enabled("music_enabled", !_get_option_enabled("music_enabled", true));
            break;
        case 1:
            _set_option_enabled("sfx_enabled", !_get_option_enabled("sfx_enabled", true));
            break;
        case 2:
            _set_option_enabled("show_notifications", !_get_option_enabled("show_notifications", true));
            break;
        case 3:
            _set_modal_screen("controls", modal_from_pause);
            return;
        case 4:
            if (!recent_replay_files.is_empty() && _load_replay_from_file(String(recent_replay_files[0]))) {
                _close_modal_screen();
                _set_title_visible(false);
                _set_selector_visible(false);
                return;
            }
            break;
        case 5:
            for (int32_t i = 0; i < ACTION_BINDING_COUNT; i++) {
                _apply_control_binding(ACTION_BINDINGS[i].action, ACTION_BINDINGS[i].default_key);
            }
            _save_progress();
            _show_runtime_message("Touches restaurees par defaut.");
            break;
        default:
            break;
        }
    } else if (modal_screen == "controls") {
        int32_t configurable_index = 0;
        for (int32_t i = 0; i < ACTION_BINDING_COUNT; i++) {
            if (!ACTION_BINDINGS[i].configurable) {
                continue;
            }
            if (configurable_index == p_index) {
                pending_rebind_action = ACTION_BINDINGS[i].action;
                _refresh_modal_ui();
                return;
            }
            configurable_index += 1;
        }

        if (p_index == configurable_index) {
            for (int32_t i = 0; i < ACTION_BINDING_COUNT; i++) {
                _apply_control_binding(ACTION_BINDINGS[i].action, ACTION_BINDINGS[i].default_key);
            }
            _save_progress();
            _show_runtime_message("Touches restaurees par defaut.");
        } else if (p_index == configurable_index + 1) {
            _set_modal_screen("options", modal_from_pause);
            return;
        }
    }

    _refresh_modal_ui();
}

void MnmsGameDirector::_on_modal_back_pressed() {
    if (modal_screen == "controls") {
        pending_rebind_action = "";
        _set_modal_screen("options", modal_from_pause);
        return;
    }
    _close_modal_screen();
}

void MnmsGameDirector::_on_music_finished() {
    if (current_music_mode == "menu") {
        if (!menu_music_loop_path.is_empty()) {
            _play_music_stream(menu_music_loop_path, true);
        } else if (!menu_music_intro_path.is_empty()) {
            _play_music_stream(menu_music_intro_path, false);
        }
        return;
    }

    if (current_music_mode == "gameplay" && !gameplay_music_paths.is_empty()) {
        gameplay_music_index = (gameplay_music_index + 1) % gameplay_music_paths.size();
        _play_music_stream(String(gameplay_music_paths[gameplay_music_index]), false);
    }
}
