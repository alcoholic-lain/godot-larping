// src/mnms_shadow.cpp
#include "mnms_shadow.h"

#include "mnms_player.h"

#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/kinematic_collision2d.hpp>
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
constexpr int LAYER_SHADOW_WORLD = 1 << 1;
constexpr int LAYER_SHADOW_BODY = 1 << 3;
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

static void ensure_shadow_shape(CharacterBody2D *p_body) {
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

static void ensure_shadow_visual(Node2D *p_body) {
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
    poly->set_color(Color(0.45, 0.34, 0.72, 0.82));
    p_body->add_child(poly);
}

static Sprite2D *ensure_shadow_sprite(Node2D *p_body) {
    Sprite2D *sprite = Object::cast_to<Sprite2D>(p_body->get_node_or_null("VisualSprite"));
    if (sprite == nullptr) {
        sprite = memnew(Sprite2D);
        sprite->set_name("VisualSprite");
        sprite->set_as_top_level(true);
        sprite->set_z_as_relative(false);
        sprite->set_z_index(15);
        sprite->set_centered(false);
        sprite->set_region_enabled(true);
        sprite->set_region_filter_clip_enabled(true);
        sprite->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
        p_body->add_child(sprite);
    }
    return sprite;
}

static void set_shadow_fallback_visible(Node2D *p_body, bool p_visible) {
    Polygon2D *fallback = Object::cast_to<Polygon2D>(p_body->get_node_or_null("Visual"));
    if (fallback != nullptr) {
        fallback->set_visible(p_visible);
    }
}

static void clear_shadow_visual(Node2D *p_body) {
    Sprite2D *sprite = ensure_shadow_sprite(p_body);
    sprite->set_visible(false);
    set_shadow_fallback_visible(p_body, true);
}

static Ref<Texture2D> get_cached_shadow_texture(Dictionary &p_cache, const String &p_texture_path) {
    if (p_cache.has(p_texture_path)) {
        return p_cache[p_texture_path];
    }

    Ref<Texture2D> texture = ResourceLoader::get_singleton()->load(p_texture_path);
    if (texture.is_valid()) {
        p_cache[p_texture_path] = texture;
    }
    return texture;
}

static void apply_shadow_frame(Node2D *p_body, Dictionary &p_texture_cache, const Dictionary &p_state_info, int32_t p_frame_index) {
    if (!p_state_info.has("texture_path") || !p_state_info.has("frames")) {
        clear_shadow_visual(p_body);
        return;
    }

    const String texture_path = p_state_info["texture_path"];
    const Array frames = p_state_info["frames"];
    if (texture_path.is_empty() || frames.is_empty()) {
        clear_shadow_visual(p_body);
        return;
    }

    const int32_t clamped_frame = CLAMP(p_frame_index, 0, frames.size() - 1);
    Dictionary frame = frames[clamped_frame];
    Ref<Texture2D> texture = get_cached_shadow_texture(p_texture_cache, texture_path);
    if (texture.is_null()) {
        clear_shadow_visual(p_body);
        return;
    }

    Sprite2D *sprite = ensure_shadow_sprite(p_body);
    const double frame_x = frame.has("x") ? (double)frame["x"] : 0.0;
    const double frame_y = frame.has("y") ? (double)frame["y"] : 0.0;
    const double frame_w = frame.has("w") ? (double)frame["w"] : 24.0;
    const double frame_h = frame.has("h") ? (double)frame["h"] : 40.0;
    sprite->set_texture(texture);
    sprite->set_region_rect(Rect2(frame_x, frame_y, frame_w, frame_h));
    sprite->set_global_position((p_body->get_global_position() + Vector2(-Math::floor(frame_w * 0.5), -Math::floor(frame_h))).round());
    sprite->set_visible(true);
    set_shadow_fallback_visible(p_body, false);
}

} // namespace

void MnmsShadow::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_character_states", "character_states"), &MnmsShadow::set_character_states);
    ClassDB::bind_method(D_METHOD("get_character_states"), &MnmsShadow::get_character_states);
    ClassDB::bind_method(D_METHOD("set_spawn_position", "spawn_position"), &MnmsShadow::set_spawn_position);
    ClassDB::bind_method(D_METHOD("get_spawn_position"), &MnmsShadow::get_spawn_position);
    ClassDB::bind_method(D_METHOD("reset_to_spawn"), &MnmsShadow::reset_to_spawn);
    ClassDB::bind_method(D_METHOD("start_replay", "replay_frames"), &MnmsShadow::start_replay);
    ClassDB::bind_method(D_METHOD("stop_replay"), &MnmsShadow::stop_replay);
    ClassDB::bind_method(D_METHOD("is_replaying"), &MnmsShadow::is_replaying);

    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "character_states"), "set_character_states", "get_character_states");
}

MnmsShadow::MnmsShadow() {
    move_speed = 240.0;
    jump_velocity = 420.0;
    gravity = 980.0;
    current_state_name = "";
    facing_right = true;
    animation_time = 0.0;
    last_axis = 0.0;
    grounded_visual_grace = 0.0;
    walk_visual_grace = 0.0;
    visual_grounded = false;
    visual_vertical_speed = 0.0;
    visual_horizontal_speed = 0.0;
    replay_frame_index = 0;
    animation_frame_index = 0;
    replaying = false;
    supported_by_peer = false;
    spawn_position = Vector2();
    set_process_priority(10);
    set_physics_process_priority(10);
}

MnmsShadow::~MnmsShadow() {}

void MnmsShadow::set_character_states(const Dictionary &p_character_states) {
    character_states = p_character_states;
    current_state_name = "";
    animation_time = 0.0;
    animation_frame_index = 0;
    if (is_inside_tree()) {
        _update_visual_state(0.0);
    }
}

Dictionary MnmsShadow::get_character_states() const {
    return character_states;
}

void MnmsShadow::_ready() {
    add_to_group("mnms_shadow");
    ensure_shadow_shape(this);
    ensure_shadow_visual(this);
    ensure_shadow_sprite(this);
    _update_visual_state(0.0);
    set_collision_layer(LAYER_SHADOW_BODY);
    set_collision_mask(LAYER_WORLD | LAYER_SHADOW_WORLD);
    set_floor_snap_length(8.0);
    set_safe_margin(0.04);
    set_supported_by_peer_now(this, false);

    Variant g = ProjectSettings::get_singleton()->get_setting("physics/2d/default_gravity");
    if (g.get_type() == Variant::FLOAT || g.get_type() == Variant::INT) {
        gravity = (double)g;
    }
    set_process(true);
}

void MnmsShadow::_process(double delta) {
    _update_visual_state(delta);
}

void MnmsShadow::_physics_process(double delta) {
    const Vector2 previous_global_position = get_global_position();
    const bool was_supported_by_peer = supported_by_peer;
    const bool grounded_for_jump = is_on_floor() || was_supported_by_peer;
    supported_by_peer = false;
    set_supported_by_peer_now(this, false);

    if (!replaying) {
        last_axis = 0.0;
        set_velocity(Vector2());
        _resolve_player_support();
        const Vector2 actual_motion = get_global_position() - previous_global_position;
        const double actual_horizontal_speed = delta > 0.0 ? actual_motion.x / delta : 0.0;
        if (supported_by_peer) {
            walk_visual_grace = 0.0;
        } else if (Math::abs(actual_horizontal_speed) > 12.0) {
            walk_visual_grace = 0.08;
        } else if (walk_visual_grace > 0.0) {
            walk_visual_grace = MAX(0.0, walk_visual_grace - delta);
        }
        visual_grounded = is_on_floor() || supported_by_peer;
        visual_horizontal_speed = actual_horizontal_speed;
        visual_vertical_speed = get_real_velocity().y;
        return;
    }

    if (replay_frame_index >= replay_frames.size()) {
        stop_replay();
        return;
    }

    Vector2 v = get_velocity();
    double axis = 0.0;
    bool jump_pressed = false;
    const bool uses_runtime_input = has_mnms_input_frame(this);

    if (uses_runtime_input) {
        axis = (double)(int)get_meta("mnms_move_dir");
        jump_pressed = (bool)get_meta("mnms_jump_pressed");
    } else {
        Dictionary frame = replay_frames[replay_frame_index];
        if (frame.has("move_dir")) {
            axis = (double)(int)frame["move_dir"];
        } else if (frame.has("axis")) {
            axis = (double)frame["axis"];
        }
        if (frame.has("jump_pressed")) {
            jump_pressed = (bool)frame["jump_pressed"];
        }
    }

    last_axis = axis;
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

    _resolve_player_support();

    const Vector2 actual_motion = get_global_position() - previous_global_position;
    const double actual_horizontal_speed = delta > 0.0 ? actual_motion.x / delta : 0.0;

    if (is_on_floor() || supported_by_peer) {
        grounded_visual_grace = 0.08;
    } else if (grounded_visual_grace > 0.0) {
        grounded_visual_grace = MAX(0.0, grounded_visual_grace - delta);
    }
    if (supported_by_peer) {
        walk_visual_grace = 0.0;
    } else if (Math::abs(axis) > 0.02 || Math::abs(actual_horizontal_speed) > 12.0) {
        walk_visual_grace = 0.08;
    } else if (walk_visual_grace > 0.0) {
        walk_visual_grace = MAX(0.0, walk_visual_grace - delta);
    }
    visual_grounded = is_on_floor() || supported_by_peer;
    visual_horizontal_speed = actual_horizontal_speed;
    visual_vertical_speed = get_real_velocity().y;
    replay_frame_index += 1;
    if (replay_frame_index >= replay_frames.size()) {
        stop_replay();
    }
}

void MnmsShadow::set_spawn_position(const Vector2 &p_spawn_position) {
    spawn_position = p_spawn_position;
}

Vector2 MnmsShadow::get_spawn_position() const {
    return spawn_position;
}

void MnmsShadow::reset_to_spawn() {
    replaying = false;
    replay_frame_index = 0;
    animation_frame_index = 0;
    animation_time = 0.0;
    last_axis = 0.0;
    replay_frames.clear();
    set_position(spawn_position);
    set_velocity(Vector2());
    visual_grounded = false;
    visual_horizontal_speed = 0.0;
    visual_vertical_speed = 0.0;
    walk_visual_grace = 0.0;
    supported_by_peer = false;
    set_supported_by_peer_now(this, false);
}

void MnmsShadow::start_replay(const Array &p_replay_frames) {
    replay_frames = p_replay_frames;
    replay_frame_index = 0;
    animation_frame_index = 0;
    animation_time = 0.0;
    last_axis = 0.0;
    replaying = !replay_frames.is_empty();
    set_velocity(Vector2());
    visual_grounded = false;
    visual_horizontal_speed = 0.0;
    visual_vertical_speed = 0.0;
    walk_visual_grace = 0.0;
    supported_by_peer = false;
    set_supported_by_peer_now(this, false);
}

void MnmsShadow::stop_replay() {
    replaying = false;
    replay_frame_index = replay_frames.size();
    last_axis = 0.0;
    set_velocity(Vector2());
    visual_horizontal_speed = 0.0;
    walk_visual_grace = 0.0;
}

bool MnmsShadow::is_replaying() const {
    return replaying;
}

Array MnmsShadow::get_replay_frames() const {
    return replay_frames;
}

int32_t MnmsShadow::get_replay_frame_index() const {
    return replay_frame_index;
}

void MnmsShadow::restore_replay_state(const Array &p_replay_frames, int32_t p_replay_frame_index, const Vector2 &p_position, const Vector2 &p_velocity) {
    replay_frames = p_replay_frames;
    replay_frame_index = CLAMP(p_replay_frame_index, 0, MAX(0, replay_frames.size()));
    replaying = !replay_frames.is_empty() && replay_frame_index < replay_frames.size();
    set_position(p_position);
    set_velocity(p_velocity);
}

Dictionary MnmsShadow::peek_next_frame() const {
    if (!replaying || replay_frame_index < 0 || replay_frame_index >= replay_frames.size()) {
        return Dictionary();
    }
    return replay_frames[replay_frame_index];
}

void MnmsShadow::_update_visual_state(double delta) {
    if (character_states.is_empty()) {
        clear_shadow_visual(this);
        return;
    }

    const bool wants_walk = Math::abs(last_axis) > 0.02 || Math::abs(visual_horizontal_speed) > 12.0 || walk_visual_grace > 0.0;
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
        clear_shadow_visual(this);
        return;
    }

    Dictionary state_info = character_states[next_state];
    Array frames = state_info.has("frames") ? Array(state_info["frames"]) : Array();
    if (frames.is_empty()) {
        clear_shadow_visual(this);
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

    apply_shadow_frame(this, texture_cache, state_info, animation_frame_index);
}

Node2D *MnmsShadow::_get_player_peer() const {
    Node *parent = get_parent();
    if (parent == nullptr) {
        return nullptr;
    }
    return Object::cast_to<Node2D>(parent->get_node_or_null("Player"));
}

bool MnmsShadow::_resolve_player_support() {
    Node2D *peer_node = _get_player_peer();
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
