// src/mnms_player.cpp
#include "mnms_player.h"

#include "mnms_shadow.h"

#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/kinematic_collision2d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/sprite2d.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace {

constexpr int LAYER_WORLD = 1 << 0;
constexpr int LAYER_PLAYER_BODY = 1 << 2;
constexpr double BODY_HALF_WIDTH = 13.0;
constexpr double BODY_HEIGHT = 40.0;
constexpr double SUPPORT_MIN_OVERLAP = 8.0;
constexpr double SUPPORT_MAX_GAP = 14.0;
constexpr double SUPPORT_MAX_PENETRATION = 6.0;

static const char *META_SUPPORTED_BY_PEER = "mnms_supported_by_peer";
static const char *META_HOLDING_PEER_FRAME = "mnms_holding_peer_frame";

static Rect2 build_body_rect(const Vector2 &p_world_position) {
    return Rect2(p_world_position + Vector2(-BODY_HALF_WIDTH, -BODY_HEIGHT), Vector2(BODY_HALF_WIDTH * 2.0, BODY_HEIGHT));
}

static bool can_snap_body_to_position(CharacterBody2D *p_body, const Vector2 &p_target_position) {
    if (p_body == nullptr) {
        return false;
    }

    const Vector2 snap_motion = p_target_position - p_body->get_global_position();
    if (snap_motion.is_zero_approx()) {
        return true;
    }

    Ref<KinematicCollision2D> collision;
    return !p_body->test_move(p_body->get_global_transform(), snap_motion, collision, p_body->get_safe_margin(), true);
}

static int64_t get_current_physics_frame() {
    return Engine::get_singleton()->get_physics_frames();
}

static bool is_supported_by_peer_now(Node *p_node) {
    if (p_node == nullptr || !p_node->has_meta(META_SUPPORTED_BY_PEER)) {
        return false;
    }
    return (bool)p_node->get_meta(META_SUPPORTED_BY_PEER);
}

static void set_supported_by_peer_now(Node *p_node, bool p_supported) {
    if (p_node != nullptr) {
        p_node->set_meta(META_SUPPORTED_BY_PEER, p_supported);
    }
}

static void mark_holding_peer(Node *p_node) {
    if (p_node != nullptr) {
        p_node->set_meta(META_HOLDING_PEER_FRAME, get_current_physics_frame());
    }
}

static bool is_holding_peer_now(Node *p_node) {
    if (p_node == nullptr || !p_node->has_meta(META_HOLDING_PEER_FRAME)) {
        return false;
    }
    return (int64_t)p_node->get_meta(META_HOLDING_PEER_FRAME) == get_current_physics_frame();
}

static bool has_mnms_input_frame(Node *p_node) {
    if (p_node == nullptr || !p_node->has_meta("mnms_input_frame")) {
        return false;
    }

    Variant frame_variant = p_node->get_meta("mnms_input_frame");
    if (frame_variant.get_type() != Variant::DICTIONARY) {
        return false;
    }

    Dictionary frame = frame_variant;
    return !frame.is_empty();
}

static void ensure_player_shape(CharacterBody2D *p_body) {
    if (p_body->get_node_or_null("CollisionShape2D") != nullptr) {
        return;
    }

    CollisionShape2D *shape_node = memnew(CollisionShape2D);
    shape_node->set_name("CollisionShape2D");
    Ref<RectangleShape2D> rect_shape;
    rect_shape.instantiate();
    rect_shape->set_size(Vector2(26, 40));
    shape_node->set_shape(rect_shape);
    shape_node->set_position(Vector2(0, -20));
    p_body->add_child(shape_node);
}

static void ensure_player_visual(Node2D *p_body) {
    if (p_body->get_node_or_null("Visual") != nullptr) {
        return;
    }

    Polygon2D *poly = memnew(Polygon2D);
    poly->set_name("Visual");
    PackedVector2Array points;
    points.push_back(Vector2(-13, -40));
    points.push_back(Vector2(13, -40));
    points.push_back(Vector2(13, 0));
    points.push_back(Vector2(-13, 0));
    poly->set_polygon(points);
    poly->set_color(Color(0.28, 0.68, 0.98, 1.0));
    p_body->add_child(poly);
}

static Sprite2D *ensure_player_sprite(Node2D *p_body) {
    Sprite2D *sprite = Object::cast_to<Sprite2D>(p_body->get_node_or_null("VisualSprite"));
    if (sprite == nullptr) {
        sprite = memnew(Sprite2D);
        sprite->set_name("VisualSprite");
        sprite->set_as_top_level(true);
        sprite->set_z_as_relative(false);
        sprite->set_z_index(20);
        sprite->set_centered(false);
        sprite->set_region_enabled(true);
        sprite->set_region_filter_clip_enabled(true);
        sprite->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
        p_body->add_child(sprite);
    }
    return sprite;
}

static Node2D *ensure_player_trail_root(Node2D *p_body) {
    Node2D *trail_root = Object::cast_to<Node2D>(p_body->get_node_or_null("RecordingTrail"));
    if (trail_root == nullptr) {
        trail_root = memnew(Node2D);
        trail_root->set_name("RecordingTrail");
        trail_root->set_as_top_level(true);
        trail_root->set_z_as_relative(false);
        trail_root->set_z_index(5);
        p_body->add_child(trail_root);
    }
    trail_root->set_global_position(Vector2());
    return trail_root;
}

static void set_player_fallback_visible(Node2D *p_body, bool p_visible) {
    Polygon2D *fallback = Object::cast_to<Polygon2D>(p_body->get_node_or_null("Visual"));
    if (fallback != nullptr) {
        fallback->set_visible(p_visible);
    }
}

static void clear_player_visual(Node2D *p_body) {
    Sprite2D *sprite = ensure_player_sprite(p_body);
    sprite->set_visible(false);
    set_player_fallback_visible(p_body, true);
}

static Ref<Texture2D> get_cached_texture(Dictionary &p_cache, const String &p_texture_path) {
    if (p_cache.has(p_texture_path)) {
        return p_cache[p_texture_path];
    }

    Ref<Texture2D> texture = ResourceLoader::get_singleton()->load(p_texture_path);
    if (texture.is_valid()) {
        p_cache[p_texture_path] = texture;
    }
    return texture;
}

static void apply_player_frame(Node2D *p_body, Dictionary &p_texture_cache, const Dictionary &p_state_info, int32_t p_frame_index) {
    if (!p_state_info.has("texture_path") || !p_state_info.has("frames")) {
        clear_player_visual(p_body);
        return;
    }

    const String texture_path = p_state_info["texture_path"];
    const Array frames = p_state_info["frames"];
    if (texture_path.is_empty() || frames.is_empty()) {
        clear_player_visual(p_body);
        return;
    }

    const int32_t clamped_frame = CLAMP(p_frame_index, 0, frames.size() - 1);
    Dictionary frame = frames[clamped_frame];
    Ref<Texture2D> texture = get_cached_texture(p_texture_cache, texture_path);
    if (texture.is_null()) {
        clear_player_visual(p_body);
        return;
    }

    Sprite2D *sprite = ensure_player_sprite(p_body);
    const double frame_x = frame.has("x") ? (double)frame["x"] : 0.0;
    const double frame_y = frame.has("y") ? (double)frame["y"] : 0.0;
    const double frame_w = frame.has("w") ? (double)frame["w"] : 24.0;
    const double frame_h = frame.has("h") ? (double)frame["h"] : 40.0;
    sprite->set_texture(texture);
    sprite->set_region_rect(Rect2(frame_x, frame_y, frame_w, frame_h));
    sprite->set_global_position((p_body->get_global_position() + Vector2(-Math::floor(frame_w * 0.5), -Math::floor(frame_h))).round());
    sprite->set_visible(true);
    set_player_fallback_visible(p_body, false);
}

} // namespace

void MnmsPlayer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_character_states", "character_states"), &MnmsPlayer::set_character_states);
    ClassDB::bind_method(D_METHOD("get_character_states"), &MnmsPlayer::get_character_states);

    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "character_states"), "set_character_states", "get_character_states");
}

MnmsPlayer::MnmsPlayer() {
    move_speed = 240.0;
    jump_velocity = 420.0;
    gravity = 980.0;
    current_state_name = "";
    facing_right = true;
    animation_time = 0.0;
    last_input_axis = 0.0;
    grounded_visual_grace = 0.0;
    walk_visual_grace = 0.0;
    visual_grounded = false;
    visual_vertical_speed = 0.0;
    visual_horizontal_speed = 0.0;
    animation_frame_index = 0;
    supported_by_peer = false;
    recording_trail_enabled = false;
    set_process_priority(10);
    set_physics_process_priority(10);
}

MnmsPlayer::~MnmsPlayer() {}

void MnmsPlayer::set_character_states(const Dictionary &p_character_states) {
    character_states = p_character_states;
    current_state_name = "";
    animation_time = 0.0;
    animation_frame_index = 0;
    if (is_inside_tree()) {
        _ensure_recording_trail_root();
        _update_visual_state(0.0);
    }
}

Dictionary MnmsPlayer::get_character_states() const {
    return character_states;
}

void MnmsPlayer::append_recording_trail_sample() {
    if (!recording_trail_enabled) {
        return;
    }

    const Vector2 sample_position = (get_global_position() + Vector2(-1.0, -20.0)).round();
    recording_trail_points.push_back(sample_position);
    _append_recording_trail_dot(sample_position);
}

void MnmsPlayer::clear_recording_trail() {
    recording_trail_points.clear();
    Node2D *trail_root = Object::cast_to<Node2D>(get_node_or_null("RecordingTrail"));
    if (trail_root == nullptr) {
        return;
    }

    Array children = trail_root->get_children();
    for (int i = 0; i < children.size(); i++) {
        Node *child = Object::cast_to<Node>(children[i]);
        if (child != nullptr) {
            child->queue_free();
        }
    }
    trail_root->set_visible(false);
}

void MnmsPlayer::set_recording_trail_points(const Array &p_points) {
    clear_recording_trail();
    recording_trail_points = p_points.duplicate(true);
    if (recording_trail_points.is_empty()) {
        return;
    }

    _ensure_recording_trail_root();
    for (int i = 0; i < recording_trail_points.size(); i++) {
        _append_recording_trail_dot(Vector2(recording_trail_points[i]));
    }
}

Array MnmsPlayer::get_recording_trail_points() const {
    return recording_trail_points;
}

void MnmsPlayer::set_recording_trail_enabled(bool p_enabled) {
    recording_trail_enabled = p_enabled;
    _ensure_recording_trail_root();
    Node2D *trail_root = Object::cast_to<Node2D>(get_node_or_null("RecordingTrail"));
    if (trail_root != nullptr) {
        trail_root->set_visible(recording_trail_enabled && !recording_trail_points.is_empty());
    }
}

bool MnmsPlayer::is_recording_trail_enabled() const {
    return recording_trail_enabled;
}

void MnmsPlayer::_ready() {
    add_to_group("mnms_player");
    ensure_player_shape(this);
    ensure_player_visual(this);
    ensure_player_sprite(this);
    _ensure_recording_trail_root();
    _update_visual_state(0.0);
    set_collision_layer(LAYER_PLAYER_BODY);
    set_collision_mask(LAYER_WORLD);
    set_floor_snap_length(8.0);
    set_safe_margin(0.04);
    set_supported_by_peer_now(this, false);
    Variant g = ProjectSettings::get_singleton()->get_setting("physics/2d/default_gravity");
    if (g.get_type() == Variant::FLOAT || g.get_type() == Variant::INT) {
        gravity = (double)g;
    }
    set_process(true);
}

void MnmsPlayer::_process(double delta) {
    _update_visual_state(delta);
}

void MnmsPlayer::_physics_process(double delta) {
    const Vector2 previous_global_position = get_global_position();
    const bool was_supported_by_peer = supported_by_peer;
    Vector2 v = get_velocity();
    const bool grounded_for_jump = is_on_floor() || was_supported_by_peer;
    supported_by_peer = false;
    set_supported_by_peer_now(this, false);
    const bool uses_runtime_input = has_mnms_input_frame(this);
    Input *input = uses_runtime_input ? nullptr : Input::get_singleton();
    const double axis = uses_runtime_input ? (double)(int)get_meta("mnms_move_dir") : input->get_axis("ui_left", "ui_right");
    const bool jump_pressed = uses_runtime_input ? (bool)get_meta("mnms_jump_pressed") : input->is_action_just_pressed("ui_up");
    last_input_axis = axis;
    if (axis < -0.05) {
        facing_right = false;
    } else if (axis > 0.05) {
        facing_right = true;
    }
    const double move_axis = was_supported_by_peer ? 0.0 : axis;
    v.x = move_axis * move_speed;

    if (grounded_for_jump && jump_pressed) {
        v.y = -jump_velocity;
        grounded_visual_grace = 0.0;
    } else if (grounded_for_jump) {
        if (v.y > 0.0) {
            v.y = 0.0;
        }
    } else {
        v.y += gravity * delta;
    }

    set_velocity(v);
    move_and_slide();
    if (!is_on_floor() && get_velocity().y >= 0.0) {
        apply_floor_snap();
    }

    _resolve_shadow_support();

    const Vector2 actual_motion = get_global_position() - previous_global_position;
    const double actual_horizontal_speed = delta > 0.0 ? actual_motion.x / delta : 0.0;

    if (supported_by_peer) {
        walk_visual_grace = 0.0;
    } else if (Math::abs(axis) > 0.02 || Math::abs(actual_horizontal_speed) > 12.0) {
        walk_visual_grace = 0.08;
    } else if (walk_visual_grace > 0.0) {
        walk_visual_grace = MAX(0.0, walk_visual_grace - delta);
    }

    if (is_on_floor() || supported_by_peer) {
        grounded_visual_grace = 0.08;
    } else if (grounded_visual_grace > 0.0) {
        grounded_visual_grace = MAX(0.0, grounded_visual_grace - delta);
    }
    visual_grounded = is_on_floor() || supported_by_peer;
    visual_horizontal_speed = actual_horizontal_speed;
    visual_vertical_speed = get_real_velocity().y;
}

void MnmsPlayer::_update_visual_state(double delta) {
    if (character_states.is_empty()) {
        clear_player_visual(this);
        return;
    }

    const bool wants_walk = Math::abs(last_input_axis) > 0.02 || Math::abs(visual_horizontal_speed) > 12.0 || walk_visual_grace > 0.0;
    const bool visually_grounded = visual_grounded || (grounded_visual_grace > 0.0 && visual_vertical_speed >= -20.0);
    String next_state;
    if (is_holding_peer_now(this) && visually_grounded && character_states.has("holding")) {
        next_state = "holding";
    } else if (!visually_grounded) {
        next_state = visual_vertical_speed < -10.0 ? "jump" : "fall";
    } else if (supported_by_peer) {
        next_state = "stand";
    } else if (wants_walk) {
        next_state = "walk";
    } else {
        next_state = "stand";
    }
    if (next_state != "holding") {
        next_state += facing_right ? "right" : "left";
    }

    if (!character_states.has(next_state)) {
        next_state = String("stand") + (facing_right ? "right" : "left");
    }
    if (!character_states.has(next_state)) {
        clear_player_visual(this);
        return;
    }

    Dictionary state_info = character_states[next_state];
    Array frames = state_info.has("frames") ? Array(state_info["frames"]) : Array();
    if (frames.is_empty()) {
        clear_player_visual(this);
        return;
    }

    if (current_state_name != next_state) {
        current_state_name = next_state;
        animation_time = 0.0;
        animation_frame_index = 0;
    } else if (frames.size() > 1) {
        Dictionary current_frame = frames[animation_frame_index];
        double frame_duration = current_frame.has("duration") ? (double)current_frame["duration"] : 0.1;
        if (frame_duration <= 0.0) {
            frame_duration = 0.1;
        }
        animation_time += delta;
        while (animation_time >= frame_duration) {
            animation_time -= frame_duration;
            animation_frame_index += 1;
            if (animation_frame_index >= frames.size()) {
                const bool loop = !state_info.has("loop") || (bool)state_info["loop"];
                animation_frame_index = loop ? 0 : frames.size() - 1;
            }
            current_frame = frames[animation_frame_index];
            frame_duration = current_frame.has("duration") ? (double)current_frame["duration"] : 0.1;
            if (frame_duration <= 0.0) {
                frame_duration = 0.1;
            }
        }
    } else {
        animation_frame_index = 0;
    }

    apply_player_frame(this, texture_cache, state_info, animation_frame_index);
}

void MnmsPlayer::_ensure_recording_trail_root() {
    Node2D *trail_root = ensure_player_trail_root(this);
    trail_root->set_visible(recording_trail_enabled && !recording_trail_points.is_empty());
}

Dictionary MnmsPlayer::_get_line_state_info() const {
    if (!character_states.has("line")) {
        return Dictionary();
    }
    return character_states["line"];
}

void MnmsPlayer::_append_recording_trail_dot(const Vector2 &p_world_position) {
    Node2D *trail_root = ensure_player_trail_root(this);
    trail_root->set_visible(recording_trail_enabled && !recording_trail_points.is_empty());

    const Dictionary line_state = _get_line_state_info();
    const Array frames = line_state.has("frames") ? Array(line_state["frames"]) : Array();
    const Dictionary frame = frames.is_empty() ? Dictionary() : Dictionary(frames[0]);
    const String texture_path = line_state.has("texture_path") ? String(line_state["texture_path"]) : String();
    const Ref<Texture2D> texture = texture_path.is_empty() ? Ref<Texture2D>() : get_cached_texture(texture_cache, texture_path);

    if (texture.is_valid()) {
        Sprite2D *dot = memnew(Sprite2D);
        dot->set_name("TrailDot");
        dot->set_centered(false);
        dot->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
        dot->set_region_enabled(true);
        dot->set_region_filter_clip_enabled(true);
        dot->set_texture(texture);
        dot->set_position(p_world_position.round());
        const double frame_x = frame.has("x") ? (double)frame["x"] : 0.0;
        const double frame_y = frame.has("y") ? (double)frame["y"] : 0.0;
        const double frame_w = frame.has("w") ? (double)frame["w"] : 5.0;
        const double frame_h = frame.has("h") ? (double)frame["h"] : 5.0;
        dot->set_region_rect(Rect2(frame_x, frame_y, frame_w, frame_h));
        trail_root->add_child(dot);
        return;
    }

    Polygon2D *fallback = memnew(Polygon2D);
    fallback->set_name("TrailDotFallback");
    PackedVector2Array points;
    points.push_back(Vector2(0, 0));
    points.push_back(Vector2(5, 0));
    points.push_back(Vector2(5, 5));
    points.push_back(Vector2(0, 5));
    fallback->set_polygon(points);
    fallback->set_color(Color(0.28, 0.68, 0.98, 0.7));
    fallback->set_position(p_world_position.round());
    trail_root->add_child(fallback);
}

Node2D *MnmsPlayer::_get_shadow_peer() const {
    Node *parent = get_parent();
    if (parent == nullptr) {
        return nullptr;
    }
    return Object::cast_to<Node2D>(parent->get_node_or_null("Shadow"));
}

bool MnmsPlayer::_resolve_shadow_support() {
    Node2D *peer_node = _get_shadow_peer();
    CharacterBody2D *peer_body = Object::cast_to<CharacterBody2D>(peer_node);
    if (peer_body == nullptr) {
        return false;
    }

    const Rect2 self_rect = build_body_rect(get_global_position());
    const Rect2 peer_rect = build_body_rect(peer_body->get_global_position());
    const double horizontal_overlap = MIN(self_rect.get_end().x, peer_rect.get_end().x) - MAX(self_rect.position.x, peer_rect.position.x);
    if (horizontal_overlap < SUPPORT_MIN_OVERLAP) {
        return false;
    }

    const double vertical_gap = peer_rect.position.y - self_rect.get_end().y;
    const bool descending_or_resting = get_real_velocity().y >= -10.0;
    const bool peer_stable = peer_body->is_on_floor() || is_supported_by_peer_now(peer_body);
    if (!descending_or_resting || !peer_stable || vertical_gap < -SUPPORT_MAX_PENETRATION || vertical_gap > SUPPORT_MAX_GAP) {
        return false;
    }

    Vector2 target_position = get_global_position();
    target_position.x += peer_body->get_position_delta().x;
    target_position.y = peer_body->get_global_position().y - BODY_HEIGHT;
    target_position = target_position.round();
    if (!can_snap_body_to_position(this, target_position)) {
        return false;
    }
    set_global_position(target_position);

    Vector2 velocity = get_velocity();
    if (velocity.y > 0.0) {
        velocity.y = 0.0;
    }
    set_velocity(velocity);
    supported_by_peer = true;
    set_supported_by_peer_now(this, true);
    mark_holding_peer(peer_body);
    return true;
}
