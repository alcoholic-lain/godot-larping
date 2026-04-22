// src/mnms_importer.cpp
#include "mnms_importer.h"

#include "mnms_tile_resource.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace {

static String sanitize_name(const String &p_input) {
    String out;
    for (int i = 0; i < p_input.length(); i++) {
        const char32_t c = p_input[i];
        const bool is_alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        const bool is_digit = (c >= '0' && c <= '9');
        if (is_alpha || is_digit) {
            out += String::chr(c);
        } else {
            out += "_";
        }
    }
    while (out.find("__") >= 0) {
        out = out.replace("__", "_");
    }
    out = out.strip_edges();
    if (out.is_empty()) {
        out = "unnamed";
    }
    return out.to_lower();
}

static bool ensure_res_dir(const String &p_res_dir) {
    const String abs_dir = ProjectSettings::get_singleton()->globalize_path(p_res_dir);
    const Error err = DirAccess::make_dir_recursive_absolute(abs_dir);
    return err == OK || err == ERR_ALREADY_EXISTS;
}

static String get_between(const String &p_text, const String &p_begin, const String &p_end) {
    const int64_t begin = p_text.find(p_begin);
    if (begin < 0) {
        return "";
    }
    const int64_t from = begin + p_begin.length();
    const int64_t end = p_text.find(p_end, from);
    if (end < 0) {
        return "";
    }
    return p_text.substr(from, end - from);
}

static Vector<String> extract_quoted_tokens(const String &p_line) {
    Vector<String> tokens;
    String current;
    bool in_quotes = false;
    for (int i = 0; i < p_line.length(); i++) {
        const char32_t c = p_line[i];
        if (c == '"') {
            if (in_quotes) {
                tokens.push_back(current);
                current = "";
                in_quotes = false;
            } else {
                in_quotes = true;
            }
            continue;
        }
        if (in_quotes) {
            current += String::chr(c);
        }
    }
    return tokens;
}

static String parse_assignment_value(const String &p_line) {
    const int64_t eq = p_line.find("=");
    if (eq < 0) {
        return "";
    }
    String value = p_line.substr(eq + 1, p_line.length() - eq - 1).strip_edges();
    if (value.begins_with("\"") && value.ends_with("\"") && value.length() >= 2) {
        value = value.substr(1, value.length() - 2);
    }
    return value;
}

static Vector<String> parse_call_args(const String &p_line, const String &p_prefix) {
    Vector<String> values;
    const int64_t begin = p_line.find(p_prefix);
    if (begin != 0) {
        return values;
    }

    const int64_t open = p_line.find("(");
    const int64_t close = p_line.rfind(")");
    if (open < 0 || close < 0 || close <= open) {
        return values;
    }

    PackedStringArray raw = p_line.substr(open + 1, close - open - 1).split(",", false);
    for (int i = 0; i < raw.size(); i++) {
        String token = raw[i].strip_edges();
        if (token.begins_with("\"") && token.ends_with("\"") && token.length() >= 2) {
            token = token.substr(1, token.length() - 2);
        }
        values.push_back(token);
    }
    return values;
}

static Dictionary make_animation_frame(const Vector<String> &p_args, double p_duration) {
    Dictionary frame;
    frame["x"] = p_args[0].to_float();
    frame["y"] = p_args[1].to_float();
    frame["w"] = p_args[2].to_float();
    frame["h"] = p_args[3].to_float();
    frame["duration"] = p_duration;
    return frame;
}

static bool is_block_open_line(const String &p_line) {
    return p_line.ends_with("{");
}

static String infer_pack_theme_id(const String &p_pack_id) {
    if (p_pack_id == "classic") {
        return "classic";
    }
    return "cloudscape";
}

} // namespace

void MnmsImporter::_bind_methods() {
    ClassDB::bind_method(D_METHOD("import_all", "source_root", "output_root"), &MnmsImporter::import_all);
    ClassDB::bind_method(D_METHOD("get_last_error"), &MnmsImporter::get_last_error);
}

MnmsImporter::MnmsImporter() {}

MnmsImporter::~MnmsImporter() {}

bool MnmsImporter::_parse_size_value(const String &p_value, Vector2i &r_size) const {
    PackedStringArray parts = p_value.split(",", false);
    if (parts.size() < 2) {
        return false;
    }

    r_size = Vector2i(parts[0].strip_edges().to_int(), parts[1].strip_edges().to_int());
    return true;
}

bool MnmsImporter::import_all(const String &p_source_root, const String &p_output_root) {
    last_error = "";

    Ref<DirAccess> source_dir = DirAccess::open(p_source_root);
    if (source_dir.is_null()) {
        last_error = "Source root introuvable: " + p_source_root;
        return false;
    }

    if (!ensure_res_dir(p_output_root)) {
        last_error = "Impossible de creer le dossier de sortie: " + p_output_root;
        return false;
    }
    if (!ensure_res_dir(p_output_root.path_join("packs")) ||
        !ensure_res_dir(p_output_root.path_join("levels")) ||
        !ensure_res_dir(p_output_root.path_join("themes"))) {
        last_error = "Impossible de creer les sous-dossiers de sortie.";
        return false;
    }

    Array pack_paths;
    Array theme_paths;
    if (!_import_levelpacks(p_source_root, p_output_root, pack_paths)) {
        return false;
    }
    if (!_import_themes(p_source_root, p_output_root, theme_paths)) {
        return false;
    }

    UtilityFunctions::print("MNMS import termine. Packs: ", pack_paths.size(), " Themes: ", theme_paths.size());
    return true;
}

String MnmsImporter::get_last_error() const {
    return last_error;
}

bool MnmsImporter::_import_levelpacks(const String &p_source_root, const String &p_output_root, Array &r_pack_paths) {
    const String packs_root = p_source_root.path_join("levelpacks");
    Ref<DirAccess> packs_dir = DirAccess::open(packs_root);
    if (packs_dir.is_null()) {
        last_error = "Dossier levelpacks introuvable: " + packs_root;
        return false;
    }

    if (packs_dir->list_dir_begin() != OK) {
        last_error = "Impossible de lire le dossier levelpacks.";
        return false;
    }

    String entry = packs_dir->get_next();
    while (!entry.is_empty()) {
        if (entry != "." && entry != ".." && packs_dir->current_is_dir()) {
            const String pack_id = sanitize_name(entry);
            const String levels_lst = packs_root.path_join(entry).path_join("levels.lst");
            if (FileAccess::file_exists(levels_lst)) {
                Ref<MnmsPackResource> pack = _parse_pack(pack_id, levels_lst, p_output_root);
                if (pack.is_valid()) {
                    String pack_file = pack_id;
                    pack_file += String("_pack.tres");
                    const String out_pack_path = p_output_root.path_join("packs").path_join(pack_file);
                    const Error save_err = ResourceSaver::get_singleton()->save(pack, out_pack_path);
                    if (save_err != OK) {
                        last_error = "Echec de sauvegarde du pack: " + out_pack_path;
                        packs_dir->list_dir_end();
                        return false;
                    }
                    r_pack_paths.push_back(out_pack_path);
                }
            }
        }
        entry = packs_dir->get_next();
    }
    packs_dir->list_dir_end();
    return true;
}

bool MnmsImporter::_import_themes(const String &p_source_root, const String &p_output_root, Array &r_theme_paths) {
    const String themes_root = p_source_root.path_join("themes");
    Ref<DirAccess> themes_dir = DirAccess::open(themes_root);
    if (themes_dir.is_null()) {
        return true;
    }

    if (themes_dir->list_dir_begin() != OK) {
        last_error = "Impossible de lire le dossier themes.";
        return false;
    }

    String entry = themes_dir->get_next();
    while (!entry.is_empty()) {
        if (entry != "." && entry != ".." && themes_dir->current_is_dir()) {
            const String theme_id = sanitize_name(entry);
            const String theme_file = themes_root.path_join(entry).path_join("theme.mnmstheme");
            if (FileAccess::file_exists(theme_file)) {
                Ref<MnmsThemeResource> theme = _parse_theme(theme_id, theme_file);
                if (theme.is_valid()) {
                    String theme_file_name = theme_id;
                    theme_file_name += String(".tres");
                    const String out_theme_path = p_output_root.path_join("themes").path_join(theme_file_name);
                    const Error save_err = ResourceSaver::get_singleton()->save(theme, out_theme_path);
                    if (save_err != OK) {
                        last_error = "Echec de sauvegarde du theme: " + out_theme_path;
                        themes_dir->list_dir_end();
                        return false;
                    }
                    r_theme_paths.push_back(out_theme_path);
                }
            }
        }
        entry = themes_dir->get_next();
    }
    themes_dir->list_dir_end();
    return true;
}

Ref<MnmsPackResource> MnmsImporter::_parse_pack(const String &p_pack_id, const String &p_levels_lst_path, const String &p_output_root) {
    Ref<FileAccess> file = FileAccess::open(p_levels_lst_path, FileAccess::READ);
    if (file.is_null()) {
        return Ref<MnmsPackResource>();
    }

    const String levels_dir_out = p_output_root.path_join("levels").path_join(p_pack_id);
    ensure_res_dir(levels_dir_out);

    Ref<MnmsPackResource> pack;
    pack.instantiate();
    pack->set_pack_id(p_pack_id);
    pack->set_name(p_pack_id);
    pack->set_description("");
    pack->set_theme_id(infer_pack_theme_id(p_pack_id));
    pack->set_music_list("");
    pack->set_congratulations("");

    const String map_root = p_levels_lst_path.get_base_dir();
    Dictionary pack_metadata;
    pack_metadata["source_levels_lst_path"] = p_levels_lst_path;
    pack_metadata["source_pack_path"] = map_root;

    Array levels;
    int32_t level_index = 0;

    while (!file->eof_reached()) {
        String line = file->get_line().strip_edges();
        if (line.is_empty()) {
            continue;
        }

        if (line.begins_with("name=")) {
            pack->set_name(parse_assignment_value(line));
            continue;
        }
        if (line.begins_with("description=")) {
            pack->set_description(parse_assignment_value(line));
            continue;
        }
        if (line.begins_with("congratulations=")) {
            pack->set_congratulations(parse_assignment_value(line));
            continue;
        }
        if (line.begins_with("musiclist=")) {
            pack->set_music_list(parse_assignment_value(line));
            continue;
        }
        if (line.begins_with("theme=")) {
            pack->set_theme_id(sanitize_name(parse_assignment_value(line)));
            continue;
        }
        if (line.begins_with("levelfile(")) {
            const Vector<String> levelfile_args = parse_call_args(line, "levelfile");
            if (levelfile_args.is_empty()) {
                continue;
            }
            const String map_name = levelfile_args[0];

            const String map_path = map_root.path_join(map_name);
            Ref<MnmsLevelResource> level = _parse_map(map_path);
            if (level.is_null()) {
                continue;
            }
            if (levelfile_args.size() >= 2 && !levelfile_args[1].is_empty()) {
                level->set_level_name(levelfile_args[1]);
            }

            Dictionary level_metadata = level->get_metadata();
            level_metadata["source_pack_id"] = p_pack_id;
            level_metadata["source_pack_path"] = map_root;
            level_metadata["source_map_name"] = map_name;
            level->set_metadata(level_metadata);

            String level_file_name = sanitize_name(level->get_level_name());
            if (level_file_name.is_empty()) {
                level_file_name = "level_" + String::num_int64(level_index);
            }
            const String level_out_path = levels_dir_out.path_join(String::num_int64(level_index) + "_" + level_file_name + ".tres");
            if (ResourceSaver::get_singleton()->save(level, level_out_path) == OK) {
                levels.push_back(level);
            }
            level_index++;
        }
    }

    pack->set_levels(levels);
    pack->set_metadata(pack_metadata);
    return pack;
}

Ref<MnmsLevelResource> MnmsImporter::_parse_map(const String &p_map_path) {
    Ref<FileAccess> file = FileAccess::open(p_map_path, FileAccess::READ);
    if (file.is_null()) {
        return Ref<MnmsLevelResource>();
    }

    Ref<MnmsLevelResource> level;
    level.instantiate();
    level->set_level_name(p_map_path.get_file().get_basename());
    level->set_world_size(Vector2i(800, 600));
    level->set_time_limit(-1);
    level->set_recordings_count(-1);

    Array tiles;
    Dictionary metadata;

    while (!file->eof_reached()) {
        String line = file->get_line().strip_edges();
        if (line.is_empty()) {
            continue;
        }

        Vector<String> quoted = extract_quoted_tokens(line);

        if (line.begins_with("\"name\"") && quoted.size() >= 2) {
            level->set_level_name(quoted[1]);
            metadata["name"] = quoted[1];
            continue;
        }
        if (line.begins_with("name=")) {
            const String value = parse_assignment_value(line);
            level->set_level_name(value);
            metadata["name"] = value;
            continue;
        }

        if (line.begins_with("\"size\"") && quoted.size() >= 3) {
            const Vector2i world_size(quoted[1].to_int(), quoted[2].to_int());
            level->set_world_size(world_size);
            metadata["size"] = String::num_int64(world_size.x) + "," + String::num_int64(world_size.y);
            continue;
        }
        if (line.begins_with("size=")) {
            Vector2i world_size;
            if (_parse_size_value(parse_assignment_value(line), world_size)) {
                level->set_world_size(world_size);
                metadata["size"] = String::num_int64(world_size.x) + "," + String::num_int64(world_size.y);
            }
            continue;
        }

        if (line.begins_with("time=")) {
            const String value = parse_assignment_value(line);
            level->set_time_limit(value.to_int());
            metadata["time"] = value;
            continue;
        }

        if (line.begins_with("recordings=")) {
            const String value = parse_assignment_value(line);
            level->set_recordings_count(value.to_int());
            metadata["recordings"] = value;
            continue;
        }

        const int64_t assignment_eq = line.find("=");
        if (assignment_eq > 0 && !line.begins_with("tile(") && !line.begins_with("\"tile\"")) {
            const String key = line.substr(0, assignment_eq).strip_edges();
            const String value = parse_assignment_value(line);
            metadata[key] = value;
            continue;
        }

        if (line.begins_with("\"tile\"") && quoted.size() >= 4) {
            Ref<MnmsTileResource> tile;
            tile.instantiate();
            tile->set_block_type(quoted[1]);
            tile->set_position(Vector2i(quoted[2].to_int(), quoted[3].to_int()));
            if (quoted.size() >= 6) {
                tile->set_size(Vector2i(quoted[4].to_int(), quoted[5].to_int()));
            } else if (quoted.size() >= 5) {
                tile->set_size(Vector2i(quoted[4].to_int(), 50));
            } else {
                tile->set_size(Vector2i(50, 50));
            }
            Dictionary props;
            props["source_line"] = line;
            if (is_block_open_line(line)) {
                while (!file->eof_reached()) {
                    String property_line = file->get_line().strip_edges();
                    if (property_line.is_empty()) {
                        continue;
                    }
                    if (property_line == "}") {
                        break;
                    }
                    const int64_t eq = property_line.find("=");
                    if (eq > 0) {
                        const String key = property_line.substr(0, eq).strip_edges();
                        const String value = parse_assignment_value(property_line);
                        props[key] = value;
                    }
                }
            }
            tile->set_properties(props);
            tiles.push_back(tile);
            continue;
        }

        Vector<String> args = parse_call_args(line, "tile");
        if (args.size() >= 3) {
            Ref<MnmsTileResource> tile;
            tile.instantiate();
            tile->set_block_type(args[0]);
            tile->set_position(Vector2i(args[1].to_int(), args[2].to_int()));
            if (args.size() >= 5) {
                tile->set_size(Vector2i(args[3].to_int(), args[4].to_int()));
            } else if (args.size() >= 4) {
                tile->set_size(Vector2i(args[3].to_int(), 50));
            } else {
                tile->set_size(Vector2i(50, 50));
            }
            Dictionary props;
            props["source_line"] = line;
            if (is_block_open_line(line)) {
                while (!file->eof_reached()) {
                    String property_line = file->get_line().strip_edges();
                    if (property_line.is_empty()) {
                        continue;
                    }
                    if (property_line == "}") {
                        break;
                    }
                    const int64_t eq = property_line.find("=");
                    if (eq > 0) {
                        const String key = property_line.substr(0, eq).strip_edges();
                        const String value = parse_assignment_value(property_line);
                        props[key] = value;
                    }
                }
            }
            tile->set_properties(props);
            tiles.push_back(tile);
        }
    }

    metadata["source_map_path"] = p_map_path;
    level->set_metadata(metadata);
    level->set_tiles(tiles);
    return level;
}

Ref<MnmsThemeResource> MnmsImporter::_parse_theme(const String &p_theme_id, const String &p_theme_file_path) {
    Ref<FileAccess> file = FileAccess::open(p_theme_file_path, FileAccess::READ);
    if (file.is_null()) {
        return Ref<MnmsThemeResource>();
    }

    Ref<MnmsThemeResource> theme;
    theme.instantiate();
    theme->set_theme_id(p_theme_id);
    theme->set_name(p_theme_id);

    Dictionary block_map;
    Dictionary character_map;
    Dictionary character_state_map;
    Dictionary character_animation_last_points;
    String current_block;
    String current_character;
    String current_state;
    String current_animation_state;
    String current_animation_texture;
    double current_animation_frame_duration = 0.1;

    while (!file->eof_reached()) {
        String line = file->get_line().strip_edges();
        if (line.is_empty() || line.begins_with("#")) {
            continue;
        }

        if (line.begins_with("name=")) {
            theme->set_name(parse_assignment_value(line));
            continue;
        }
        if (line.begins_with("block(")) {
            current_block = get_between(line, "block(", ")");
            current_character = "";
            current_state = "";
            current_animation_state = "";
            current_animation_texture = "";
            continue;
        }
        if (line.begins_with("character(")) {
            current_character = get_between(line, "character(", ")");
            current_block = "";
            current_state = "";
            current_animation_state = "";
            current_animation_texture = "";
            continue;
        }
        if (line.begins_with("state(")) {
            current_state = get_between(line, "state(", ")");
            current_animation_state = "";
            current_animation_texture = "";
            current_animation_frame_duration = 0.1;
            continue;
        }
        if (line == "}") {
            current_animation_state = "";
            current_animation_texture = "";
            continue;
        }
        if (!current_character.is_empty() && !current_state.is_empty() && line.begins_with("animation=")) {
            PackedStringArray animation_parts = parse_assignment_value(line).split(",", false);
            if (!animation_parts.is_empty()) {
                const double fps = animation_parts[0].strip_edges().to_float();
                if (fps > 0.0) {
                    current_animation_frame_duration = 1.0 / fps;
                }
            }
            continue;
        }
        if (!current_character.is_empty() && !current_state.is_empty() && line.find("pictureAnimation(") >= 0) {
            const String picture_expr = get_between(line, "pictureAnimation(", ")");
            PackedStringArray args = picture_expr.split(",", false);
            if (!args.is_empty()) {
                current_animation_texture = p_theme_file_path.get_base_dir().path_join(args[0].strip_edges());
                current_animation_state = current_state;

                Dictionary character_states = character_state_map.has(current_character) ? Dictionary(character_state_map[current_character]) : Dictionary();
                Dictionary state_info;
                state_info["texture_path"] = current_animation_texture;
                state_info["frames"] = Array();
                state_info["loop"] = true;
                character_states[current_state] = state_info;
                character_state_map[current_character] = character_states;
                character_animation_last_points[current_character + "::" + current_state] = Variant();
                if (!character_map.has(current_character)) {
                    character_map[current_character] = current_animation_texture;
                }
            }
            continue;
        }
        if (!current_character.is_empty() && !current_animation_state.is_empty() && line.begins_with("point(")) {
            Vector<String> args = parse_call_args(line, "point");
            if (args.size() >= 4 && character_state_map.has(current_character)) {
                Dictionary character_states = character_state_map[current_character];
                if (character_states.has(current_animation_state)) {
                    Dictionary state_info = character_states[current_animation_state];
                    Array frames = state_info.has("frames") ? Array(state_info["frames"]) : Array();
                    Dictionary current_point = make_animation_frame(args, current_animation_frame_duration);
                    const String last_point_key = current_character + "::" + current_animation_state;
                    Variant last_point_variant = character_animation_last_points.has(last_point_key) ? character_animation_last_points[last_point_key] : Variant();
                    int32_t frame_count = 1;
                    if (args.size() >= 5) {
                        frame_count = MAX(1, args[4].to_int());
                    }
                    if (last_point_variant.get_type() == Variant::DICTIONARY) {
                        Dictionary last_point = last_point_variant;
                        for (int32_t step = 1; step <= frame_count; step++) {
                            const double t = (double)step / (double)frame_count;
                            Dictionary interpolated_frame;
                            interpolated_frame["x"] = Math::round((double)last_point["x"] + ((double)current_point["x"] - (double)last_point["x"]) * t);
                            interpolated_frame["y"] = Math::round((double)last_point["y"] + ((double)current_point["y"] - (double)last_point["y"]) * t);
                            interpolated_frame["w"] = Math::round((double)last_point["w"] + ((double)current_point["w"] - (double)last_point["w"]) * t);
                            interpolated_frame["h"] = Math::round((double)last_point["h"] + ((double)current_point["h"] - (double)last_point["h"]) * t);
                            interpolated_frame["duration"] = current_animation_frame_duration;
                            frames.push_back(interpolated_frame);
                        }
                    } else {
                        frames.push_back(current_point);
                    }
                    character_animation_last_points[last_point_key] = current_point;
                    state_info["frames"] = frames;
                    character_states[current_animation_state] = state_info;
                    character_state_map[current_character] = character_states;
                }
            }
            continue;
        }
        if (line.find("picture(") >= 0) {
            String picture_expr = get_between(line, "picture(", ")");
            if (picture_expr.is_empty()) {
                continue;
            }
            PackedStringArray args = picture_expr.split(",", false);
            if (args.is_empty()) {
                continue;
            }
            const String texture_path = args[0].strip_edges();
            if (!current_block.is_empty() && !block_map.has(current_block)) {
                block_map[current_block] = texture_path;
            } else if (!current_character.is_empty() && !current_state.is_empty() && args.size() >= 5) {
                Dictionary character_states = character_state_map.has(current_character) ? Dictionary(character_state_map[current_character]) : Dictionary();
                Dictionary state_info;
                Array frames;
                Vector<String> frame_args;
                frame_args.push_back(args[1].strip_edges());
                frame_args.push_back(args[2].strip_edges());
                frame_args.push_back(args[3].strip_edges());
                frame_args.push_back(args[4].strip_edges());
                frames.push_back(make_animation_frame(frame_args, 0.1));
                const String resolved_texture_path = p_theme_file_path.get_base_dir().path_join(texture_path);
                state_info["texture_path"] = resolved_texture_path;
                state_info["frames"] = frames;
                state_info["loop"] = false;
                character_states[current_state] = state_info;
                character_state_map[current_character] = character_states;
                if (!character_map.has(current_character)) {
                    character_map[current_character] = resolved_texture_path;
                }
            } else if (!current_character.is_empty() && !character_map.has(current_character)) {
                character_map[current_character] = p_theme_file_path.get_base_dir().path_join(texture_path);
            }
        }
    }

    Dictionary metadata;
    metadata["source_theme_path"] = p_theme_file_path;

    theme->set_block_texture_map(block_map);
    theme->set_character_texture_map(character_map);
    theme->set_character_state_map(character_state_map);
    theme->set_metadata(metadata);
    return theme;
}
