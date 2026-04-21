// src/mnms_game_director.h
#ifndef MNMS_GAME_DIRECTOR_H
#define MNMS_GAME_DIRECTOR_H

#include "mnms_importer.h"
#include "mnms_level_runner.h"
#include "mnms_player_shadow_system.h"
#include "mnms_replay_system.h"
#include "mnms_theme_resource.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/font_file.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/texture_rect.hpp>

namespace godot {

class MnmsGameDirector : public Node2D {
    GDCLASS(MnmsGameDirector, Node2D)

private:
    Ref<MnmsImporter> importer;
    MnmsLevelRunner *level_runner;
    MnmsReplaySystem *replay_system;
    MnmsPlayerShadowSystem *player_shadow_system;
    CanvasLayer *ui_layer;
    Control *title_root;
    PanelContainer *title_panel;
    Label *title_label;
    ItemList *title_menu_list;
    Label *title_help_label;
    TextureRect *title_logo_rect;
    TextureRect *title_credits_rect;
    TextureRect *title_statistics_rect;
    Control *selector_root;
    PanelContainer *selector_panel;
    Label *selector_title_label;
    Label *selector_help_label;
    Label *selector_progress_label;
    Label *selector_details_label;
    ItemList *selector_pack_list;
    ItemList *selector_level_list;
    Button *selector_play_button;
    Button *selector_back_button;
    Control *interlevel_root;
    PanelContainer *interlevel_panel;
    Label *interlevel_title_label;
    Label *interlevel_stats_label;
    Label *interlevel_help_label;
    TextureRect *interlevel_medal_rect;
    Button *interlevel_next_button;
    Button *interlevel_restart_button;
    Button *interlevel_save_replay_button;
    Button *interlevel_select_button;
    Control *pause_root;
    PanelContainer *pause_panel;
    Label *pause_title_label;
    Label *pause_help_label;
    Button *pause_resume_button;
    Button *pause_restart_button;
    Button *pause_checkpoint_button;
    Button *pause_select_button;
    Button *pause_save_replay_button;
    Button *pause_options_button;
    Button *pause_help_button;
    Button *pause_stats_button;
    Control *modal_root;
    PanelContainer *modal_panel;
    Label *modal_title_label;
    Label *modal_body_label;
    Label *modal_help_label;
    ItemList *modal_list;
    Button *modal_back_button;
    Label *status_label;
    Label *notification_label;
    AudioStreamPlayer *sfx_player;
    AudioStreamPlayer *music_player;
    Dictionary audio_stream_cache;
    Ref<FontFile> ui_body_font;
    Ref<FontFile> ui_title_font;
    Ref<MnmsThemeResource> pack_theme;
    Ref<MnmsThemeResource> current_theme;
    Ref<MnmsPackResource> selector_pack_resource;

    String source_data_path;
    String converted_root_path;
    String progress_file_path;
    String replay_directory_path;
    String current_pack_music_list;
    String last_saved_replay_path;
    String last_loaded_replay_path;
    String default_pack_id;
    Array available_pack_ids;
    Array current_pack_levels;
    Array selector_pack_levels;
    String current_pack_id;
    Dictionary unlocked_levels_by_pack;
    Dictionary best_times_by_level;
    Dictionary best_recordings_by_level;
    int32_t current_level_index;
    int32_t current_unlocked_level_count;
    int32_t selector_unlocked_level_count;
    int32_t selected_pack_index;
    int32_t selected_level_index;
    int32_t last_completed_time;
    int32_t last_completed_recordings;
    int32_t last_old_best_time;
    int32_t last_old_best_recordings;
    int32_t last_completed_medal;
    int32_t last_old_best_medal;
    int32_t gameplay_music_index;
    bool selector_visible;
    bool title_visible;
    bool interlevel_visible;
    bool pause_visible;
    bool restart_key_down;
    bool load_checkpoint_key_down;
    bool record_key_down;
    bool cancel_record_key_down;
    bool save_replay_key_down;
    bool shadow_view_key_down;
    bool replay_restart_key_down;
    bool replay_stop_key_down;
    bool pause_key_down;
    bool validation_prev_key_down;
    bool validation_next_key_down;
    bool title_up_key_down;
    bool title_down_key_down;
    bool title_select_key_down;
    bool title_cancel_key_down;
    bool selector_toggle_key_down;
    bool selector_left_key_down;
    bool selector_right_key_down;
    bool selector_cancel_key_down;
    String current_music_mode;
    String menu_music_intro_path;
    String menu_music_loop_path;
    String active_music_stream_path;
    String modal_screen;
    String pending_rebind_action;
    Array gameplay_music_paths;
    Array recent_replay_files;
    Dictionary options_state;
    Dictionary stats_state;
    Dictionary achievements_state;
    Dictionary controls_by_action;
    bool active_music_is_menu_loop;
    bool modal_from_pause;
    double play_time_accumulator;
    double stats_save_accumulator;

    bool _bootstrap_pipeline();
    bool _load_first_level();
    bool _scan_available_packs();
    bool _load_pack(const String &p_pack_id);
    bool _load_selector_pack(const String &p_pack_id);
    bool _load_theme(const String &p_theme_id);
    bool _load_level_at_index(int32_t p_level_index, bool p_allow_locked = false, bool p_save_progress = true);
    bool _load_selected_level();
    int32_t _find_current_classic_validation_slot() const;
    bool _jump_to_classic_validation_level(int32_t p_direction);
    String _get_classic_validation_status() const;
    void _load_progress();
    void _save_progress();
    void _ensure_ui_style_resources();
    void _apply_common_ui_style(Node *p_root);
    String _get_level_record_key(const String &p_pack_id, int32_t p_level_index) const;
    int32_t _get_best_time_for_level(const String &p_pack_id, int32_t p_level_index) const;
    int32_t _get_best_recordings_for_level(const String &p_pack_id, int32_t p_level_index) const;
    int32_t _compute_level_medal(int32_t p_time, int32_t p_target_time, int32_t p_recordings, int32_t p_target_recordings) const;
    int32_t _get_unlocked_level_count(const String &p_pack_id) const;
    void _set_unlocked_level_count(const String &p_pack_id, int32_t p_unlocked_level_count);
    String _resolve_pack_theme_id(const Ref<MnmsPackResource> &p_pack) const;
    void _ensure_title_ui();
    void _refresh_title_ui();
    void _set_title_visible(bool p_visible);
    void _activate_title_selection();
    void _ensure_selector_ui();
    void _refresh_selector_ui();
    void _ensure_interlevel_ui();
    void _refresh_interlevel_ui();
    void _set_interlevel_visible(bool p_visible);
    void _ensure_pause_ui();
    void _refresh_pause_ui();
    void _set_pause_visible(bool p_visible);
    void _ensure_modal_ui();
    void _refresh_modal_ui();
    void _set_modal_screen(const String &p_screen, bool p_from_pause = false);
    void _close_modal_screen();
    void _refresh_recent_replays();
    bool _load_replay_from_file(const String &p_path);
    void _ensure_input_actions();
    void _apply_control_binding(const String &p_action, int32_t p_keycode);
    int32_t _get_bound_keycode(const String &p_action) const;
    String _get_action_label(const String &p_action) const;
    String _get_keycode_label(int32_t p_keycode) const;
    bool _get_option_enabled(const String &p_option_key, bool p_default_value = true) const;
    void _set_option_enabled(const String &p_option_key, bool p_enabled);
    int32_t _get_stat_value(const String &p_key) const;
    void _set_stat_value(const String &p_key, int32_t p_value);
    void _add_stat(const String &p_key, int32_t p_amount = 1);
    void _refresh_achievements();
    void _unlock_achievement(const String &p_id, const String &p_title);
    String _build_help_text() const;
    String _build_credits_text() const;
    String _build_stats_text() const;
    void _show_runtime_message(const String &p_message);
    void _update_audio_settings();
    bool _can_save_replay() const;
    bool _save_current_replay();
    String _build_replay_file_path() const;
    void _refresh_music_state();
    void _play_music_stream(const String &p_path, bool p_is_menu_loop = false);
    bool _load_music_library();
    void _play_sfx(const String &p_relative_path);
    void _refresh_status_ui(int32_t p_elapsed_ticks = -1, int32_t p_recordings_used = -1, int32_t p_target_time = -1, int32_t p_target_recordings = -1);
    void _toggle_selector();
    void _set_selector_visible(bool p_visible);
    void _wire_runtime_systems();
    void _restart_current_level();
    void _load_checkpoint_or_restart();
    void _advance_to_next_level();
    void _focus_selector_control();
    void _move_selector_focus(int32_t p_direction);
    void _on_selector_pack_selected(int32_t p_index);
    void _on_selector_pack_activated(int32_t p_index);
    void _on_selector_level_selected(int32_t p_index);
    void _on_selector_level_activated(int32_t p_index);
    void _on_selector_play_pressed();
    void _on_interlevel_next_pressed();
    void _on_interlevel_restart_pressed();
    void _on_interlevel_save_replay_pressed();
    void _on_interlevel_select_pressed();
    void _on_pause_resume_pressed();
    void _on_pause_restart_pressed();
    void _on_pause_checkpoint_pressed();
    void _on_pause_select_pressed();
    void _on_pause_save_replay_pressed();
    void _on_pause_options_pressed();
    void _on_pause_help_pressed();
    void _on_pause_stats_pressed();
    void _on_modal_item_selected(int32_t p_index);
    void _on_modal_item_activated(int32_t p_index);
    void _on_modal_back_pressed();
    void _on_music_finished();

protected:
    static void _bind_methods();

public:
    MnmsGameDirector();
    ~MnmsGameDirector();

    void _ready() override;
    void _process(double delta) override;
    void _input(const Ref<InputEvent> &event) override;
    void _on_level_failed();
    void _on_level_completed(const String &p_level_name);
    void _on_title_item_selected(int32_t p_index);
    void _on_title_item_activated(int32_t p_index);
    void _on_checkpoint_activated();
    void _on_collectable_collected();
    void _on_swap_activated();
    void _on_runtime_state_changed(int32_t p_elapsed_ticks, int32_t p_recordings_used, int32_t p_target_time, int32_t p_target_recordings);
    void _on_notification_changed(const String &p_message);
};

} // namespace godot

#endif
