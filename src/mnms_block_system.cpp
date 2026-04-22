// src/mnms_block_system.cpp
#include "mnms_block_system.h"

#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/rigid_body2d.hpp>
#include <godot_cpp/classes/sprite2d.hpp>
#include <godot_cpp/classes/static_body2d.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace {

constexpr int LAYER_WORLD = 1 << 0;
constexpr int LAYER_SHADOW_WORLD = 1 << 1;
constexpr int LAYER_PLAYER_BODY = 1 << 2;
constexpr int LAYER_SHADOW_BODY = 1 << 3;

static String resolve_texture_path(const Ref<MnmsThemeResource> &p_theme, const String &p_block_type) {
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

static Polygon2D *create_rect_visual(const Vector2 &p_size, const Color &p_color, const String &p_name = String("Visual")) {
    Polygon2D *poly = memnew(Polygon2D);
    poly->set_name(p_name);
    PackedVector2Array pts;
    pts.push_back(Vector2(0, 0));
    pts.push_back(Vector2(p_size.x, 0));
    pts.push_back(Vector2(p_size.x, p_size.y));
    pts.push_back(Vector2(0, p_size.y));
    poly->set_polygon(pts);
    poly->set_color(p_color);
    return poly;
}

static CollisionShape2D *create_rect_collision(const Vector2 &p_size) {
    CollisionShape2D *shape_node = memnew(CollisionShape2D);
    Ref<RectangleShape2D> rect_shape;
    rect_shape.instantiate();
    rect_shape->set_size(p_size);
    shape_node->set_shape(rect_shape);
    shape_node->set_position(p_size * 0.5);
    return shape_node;
}

static Area2D *create_detector_area(const String &p_name, const Vector2 &p_size, const Vector2 &p_position, int p_collision_mask) {
    Area2D *area = memnew(Area2D);
    area->set_name(p_name);
    area->set_collision_layer(0);
    area->set_collision_mask(p_collision_mask);
    area->set_monitoring(true);
    area->add_child(create_rect_collision(p_size));
    area->set_position(p_position);
    return area;
}

static void apply_moving_path_properties(Node2D *p_node, const Ref<MnmsTileResource> &p_tile) {
    if (p_node == nullptr || p_tile.is_null()) {
        return;
    }

    const Dictionary props = p_tile->get_properties();
    if (!props.has("MovingPosCount")) {
        return;
    }

    const int32_t point_count = String(props["MovingPosCount"]).to_int();
    Array points;
    double total_time = 0.0;
    for (int32_t i = 0; i < point_count; i++) {
        Dictionary point;
        const String suffix = String::num_int64(i);
        const String x_key = "x" + suffix;
        const String y_key = "y" + suffix;
        const String t_key = "t" + suffix;
        point["x"] = props.has(x_key) ? String(props[x_key]).to_float() : 0.0;
        point["y"] = props.has(y_key) ? String(props[y_key]).to_float() : 0.0;
        point["t"] = props.has(t_key) ? MAX(0.0, String(props[t_key]).to_float()) : 0.0;
        total_time += (double)point["t"];
        points.push_back(point);
    }

    bool loops = true;
    if (props.has("loop")) {
        const String loop_value = String(props["loop"]).to_lower();
        loops = loop_value == "1" || loop_value == "true" || loop_value == "yes";
    }

    p_node->set_meta("moving_base_position", p_node->get_position());
    p_node->set_meta("moving_points", points);
    p_node->set_meta("moving_total_time", total_time);
    p_node->set_meta("moving_time", 0.0);
    p_node->set_meta("moving_loops", loops);
}

static void apply_common_tile_properties(Node *p_node, const Ref<MnmsTileResource> &p_tile, bool p_default_enabled = true) {
    if (p_node == nullptr || p_tile.is_null()) {
        return;
    }

    const Dictionary props = p_tile->get_properties();
    p_node->set_meta("tile_properties", props);
    p_node->set_meta("tile_size", Vector2(p_tile->get_size()));
    p_node->set_meta("enabled", p_default_enabled);

    if (props.has("id")) {
        p_node->set_meta("control_id", String(props["id"]));
    }
    if (props.has("behaviour")) {
        p_node->set_meta("behaviour", String(props["behaviour"]).to_lower());
    }
    if (props.has("destination")) {
        p_node->set_meta("destination", String(props["destination"]));
    }
    if (props.has("automatic")) {
        const String automatic = String(props["automatic"]).to_lower();
        const bool is_automatic = automatic == "1" || automatic == "true" || automatic == "yes";
        p_node->set_meta("automatic", is_automatic);
    }
    if (props.has("activated")) {
        const String activated = String(props["activated"]).to_lower();
        const bool is_active = activated == "1" || activated == "true" || activated == "yes";
        p_node->set_meta("enabled", is_active);
    }
    if (props.has("disabled")) {
        const String disabled = String(props["disabled"]).to_lower();
        const bool is_disabled = disabled == "1" || disabled == "true" || disabled == "yes";
        p_node->set_meta("enabled", !is_disabled);
    }
}

static Node *create_themed_visual(const Ref<MnmsThemeResource> &p_theme, const String &p_block_type, const Vector2 &p_size, const Color &p_fallback_color) {
    const String texture_path = resolve_texture_path(p_theme, p_block_type);
    if (!texture_path.is_empty()) {
        Ref<Texture2D> texture = ResourceLoader::get_singleton()->load(texture_path);
        if (texture.is_valid()) {
            Sprite2D *sprite = memnew(Sprite2D);
            sprite->set_name("Visual");
            sprite->set_texture(texture);
            sprite->set_centered(false);

            const Vector2 texture_size = texture->get_size();
            if (texture_size.x > 0.0 && texture_size.y > 0.0) {
                sprite->set_scale(Vector2(p_size.x / texture_size.x, p_size.y / texture_size.y));
            }
            return sprite;
        }
    }

    return create_rect_visual(p_size, p_fallback_color);
}

} // namespace

void MnmsBlockSystem::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_theme", "theme"), &MnmsBlockSystem::set_theme);
    ClassDB::bind_method(D_METHOD("get_theme"), &MnmsBlockSystem::get_theme);
    ClassDB::bind_method(D_METHOD("spawn_tile", "tile", "parent"), &MnmsBlockSystem::spawn_tile);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "theme", PROPERTY_HINT_RESOURCE_TYPE, "MnmsThemeResource"), "set_theme", "get_theme");
}

MnmsBlockSystem::MnmsBlockSystem() {}

MnmsBlockSystem::~MnmsBlockSystem() {}

void MnmsBlockSystem::set_theme(const Ref<MnmsThemeResource> &p_theme) {
    theme = p_theme;
}

Ref<MnmsThemeResource> MnmsBlockSystem::get_theme() const {
    return theme;
}

Node *MnmsBlockSystem::spawn_tile(const Ref<MnmsTileResource> &p_tile, Node *p_parent) {
    if (p_tile.is_null() || p_parent == nullptr) {
        return nullptr;
    }

    const String block_type = p_tile->get_block_type();
    const Vector2 tile_position = Vector2(p_tile->get_position());
    const Vector2 tile_size = Vector2(p_tile->get_size());

    if (block_type == "Block" || block_type == "ShadowBlock") {
        StaticBody2D *body = memnew(StaticBody2D);
        body->set_position(tile_position);
        body->set_collision_layer(block_type == "Block" ? LAYER_WORLD : LAYER_SHADOW_WORLD);
        body->set_collision_mask(0);
        body->add_child(create_rect_collision(tile_size));
        body->add_child(create_themed_visual(theme, block_type, tile_size, block_type == "Block" ? Color(0.27, 0.47, 0.78, 1.0) : Color(0.34, 0.32, 0.64, 1.0)));
        body->set_meta("default_collision_layer", body->get_collision_layer());
        body->set_meta("default_collision_mask", body->get_collision_mask());
        apply_common_tile_properties(body, p_tile);
        body->add_to_group(block_type == "Block" ? "mnms_solid" : "mnms_shadow_solid");
        p_parent->add_child(body);
        return body;
    }

    if (block_type == "MovingBlock" || block_type == "MovingShadowBlock") {
        const bool shadow_only = block_type == "MovingShadowBlock";
        StaticBody2D *body = memnew(StaticBody2D);
        body->set_position(tile_position);
        body->set_collision_layer(shadow_only ? LAYER_SHADOW_WORLD : LAYER_WORLD);
        body->set_collision_mask(0);
        body->add_child(create_rect_collision(tile_size));
        body->add_child(create_themed_visual(theme, block_type, tile_size, shadow_only ? Color(0.42, 0.36, 0.7, 1.0) : Color(0.3, 0.58, 0.85, 1.0)));
        Area2D *detector = create_detector_area("Detector", Vector2(MAX(8.0, tile_size.x - 6.0), 14.0), Vector2(tile_size.x * 0.5, -4.0), shadow_only ? LAYER_SHADOW_BODY : LAYER_PLAYER_BODY);
        body->add_child(detector);
        body->set_meta("player_inside", false);
        body->set_meta("shadow_inside", false);
        body->set_meta("default_collision_layer", body->get_collision_layer());
        body->set_meta("default_collision_mask", body->get_collision_mask());
        apply_common_tile_properties(body, p_tile);
        apply_moving_path_properties(body, p_tile);
        body->set_meta("control_mode", "motion");
        body->add_to_group("mnms_moving_platform_root");
        body->add_to_group(shadow_only ? "mnms_shadow_solid" : "mnms_solid");
        p_parent->add_child(body);
        return body;
    }

    if (block_type == "Fragile" || block_type == "ShadowFragile") {
        const bool shadow_only = block_type == "ShadowFragile";
        StaticBody2D *body = memnew(StaticBody2D);
        body->set_position(tile_position);
        body->set_collision_layer(shadow_only ? LAYER_SHADOW_WORLD : LAYER_WORLD);
        body->set_collision_mask(0);
        body->add_child(create_rect_collision(tile_size));
        body->add_child(create_themed_visual(theme, block_type, tile_size, shadow_only ? Color(0.58, 0.45, 0.76, 1.0) : Color(0.78, 0.67, 0.29, 1.0)));

        Vector2 detector_size(MAX(8.0, tile_size.x - 6.0), 12.0);
        Vector2 detector_position(tile_size.x * 0.5, -4.0);
        Area2D *detector = create_detector_area("Detector", detector_size, detector_position, shadow_only ? LAYER_SHADOW_BODY : LAYER_PLAYER_BODY);
        body->add_child(detector);

        body->set_meta("fragile_shadow_only", shadow_only);
        body->set_meta("fragile_stage", 0);
        body->set_meta("fragile_progress", 0.0);
        body->set_meta("fragile_active_bodies", 0);
        body->set_meta("fragile_broken", false);
        body->set_meta("default_collision_layer", body->get_collision_layer());
        body->set_meta("default_collision_mask", body->get_collision_mask());
        apply_common_tile_properties(body, p_tile);
        body->add_to_group(shadow_only ? "mnms_shadow_fragile_root" : "mnms_fragile_root");
        p_parent->add_child(body);
        return body;
    }

    if (block_type == "Spikes") {
        Area2D *area = memnew(Area2D);
        area->set_position(tile_position);
        area->set_collision_layer(0);
        area->set_collision_mask(LAYER_PLAYER_BODY | LAYER_SHADOW_BODY);
        area->add_child(create_rect_collision(tile_size));
        area->add_child(create_themed_visual(theme, block_type, tile_size, Color(0.82, 0.2, 0.2, 1.0)));
        area->set_meta("default_collision_layer", area->get_collision_layer());
        area->set_meta("default_collision_mask", area->get_collision_mask());
        apply_common_tile_properties(area, p_tile);
        area->add_to_group("mnms_spikes");
        p_parent->add_child(area);
        return area;
    }

    if (block_type == "MovingSpikes" || block_type == "MovingShadowSpikes") {
        const bool shadow_only = block_type == "MovingShadowSpikes";
        Area2D *area = memnew(Area2D);
        area->set_position(tile_position);
        area->set_collision_layer(0);
        area->set_collision_mask(shadow_only ? LAYER_SHADOW_BODY : (LAYER_PLAYER_BODY | LAYER_SHADOW_BODY));
        area->add_child(create_rect_collision(tile_size));
        area->add_child(create_themed_visual(theme, block_type, tile_size, shadow_only ? Color(0.65, 0.18, 0.75, 1.0) : Color(0.85, 0.24, 0.24, 1.0)));
        area->set_meta("default_collision_layer", area->get_collision_layer());
        area->set_meta("default_collision_mask", area->get_collision_mask());
        apply_common_tile_properties(area, p_tile);
        apply_moving_path_properties(area, p_tile);
        area->set_meta("control_mode", "motion");
        area->add_to_group("mnms_moving_hazard_root");
        area->add_to_group(shadow_only ? "mnms_shadow_spikes" : "mnms_spikes");
        p_parent->add_child(area);
        return area;
    }

    if (block_type == "ConveyorBelt" || block_type == "ShadowConveyorBelt") {
        const bool shadow_only = block_type == "ShadowConveyorBelt";
        StaticBody2D *body = memnew(StaticBody2D);
        body->set_position(tile_position);
        body->set_collision_layer(shadow_only ? LAYER_SHADOW_WORLD : LAYER_WORLD);
        body->set_collision_mask(0);
        body->add_child(create_rect_collision(tile_size));
        body->add_child(create_themed_visual(theme, block_type, tile_size, shadow_only ? Color(0.44, 0.46, 0.78, 1.0) : Color(0.46, 0.64, 0.9, 1.0)));
        Area2D *detector = create_detector_area("Detector", Vector2(MAX(8.0, tile_size.x - 6.0), 14.0), Vector2(tile_size.x * 0.5, -4.0), shadow_only ? LAYER_SHADOW_BODY : LAYER_PLAYER_BODY);
        body->add_child(detector);
        body->set_meta("player_inside", false);
        body->set_meta("shadow_inside", false);
        body->set_meta("default_collision_layer", body->get_collision_layer());
        body->set_meta("default_collision_mask", body->get_collision_mask());
        apply_common_tile_properties(body, p_tile);
        const Dictionary props = p_tile->get_properties();
        const double raw_speed = props.has("speed10") ? String(props["speed10"]).to_float() / 10.0 : (props.has("speed") ? String(props["speed"]).to_float() / 10.0 : 0.0);
        body->set_meta("conveyor_speed_px_per_frame", raw_speed);
        body->set_meta("control_mode", "motion");
        body->add_to_group("mnms_conveyor_root");
        body->add_to_group(shadow_only ? "mnms_shadow_solid" : "mnms_solid");
        p_parent->add_child(body);
        return body;
    }

    if (block_type == "Swap") {
        Area2D *area = memnew(Area2D);
        area->set_position(tile_position);
        area->set_collision_layer(0);
        area->set_collision_mask(LAYER_PLAYER_BODY | LAYER_SHADOW_BODY);
        area->add_child(create_rect_collision(tile_size));
        area->add_child(create_themed_visual(theme, block_type, tile_size, Color(0.23, 0.87, 0.92, 1.0)));
        area->set_meta("player_inside", false);
        area->set_meta("shadow_inside", false);
        area->set_meta("default_collision_layer", area->get_collision_layer());
        area->set_meta("default_collision_mask", area->get_collision_mask());
        apply_common_tile_properties(area, p_tile);
        area->add_to_group("mnms_swap");
        p_parent->add_child(area);
        return area;
    }

    if (block_type == "ShadowSpikes") {
        Area2D *area = memnew(Area2D);
        area->set_position(tile_position);
        area->set_collision_layer(0);
        area->set_collision_mask(LAYER_SHADOW_BODY);
        area->add_child(create_rect_collision(tile_size));
        area->add_child(create_themed_visual(theme, block_type, tile_size, Color(0.65, 0.2, 0.75, 1.0)));
        area->set_meta("default_collision_layer", area->get_collision_layer());
        area->set_meta("default_collision_mask", area->get_collision_mask());
        apply_common_tile_properties(area, p_tile);
        area->add_to_group("mnms_shadow_spikes");
        p_parent->add_child(area);
        return area;
    }

    if (block_type == "Teleporter") {
        Area2D *area = memnew(Area2D);
        area->set_position(tile_position);
        area->set_collision_layer(0);
        area->set_collision_mask(LAYER_PLAYER_BODY | LAYER_SHADOW_BODY);
        area->add_child(create_rect_collision(tile_size));
        area->add_child(create_themed_visual(theme, String("Teleporter"), tile_size, Color(0.58, 0.82, 1.0, 1.0)));
        area->set_meta("player_inside", false);
        area->set_meta("shadow_inside", false);
        area->set_meta("default_collision_layer", area->get_collision_layer());
        area->set_meta("default_collision_mask", area->get_collision_mask());
        apply_common_tile_properties(area, p_tile);
        const Dictionary props = p_tile->get_properties();
        if (props.has("message")) {
            area->set_meta("message", String(props["message"]));
        }
        area->add_to_group("mnms_teleporter");
        p_parent->add_child(area);
        return area;
    }

    if (block_type == "Switch") {
        Area2D *area = memnew(Area2D);
        area->set_position(tile_position);
        area->set_collision_layer(0);
        area->set_collision_mask(LAYER_PLAYER_BODY | LAYER_SHADOW_BODY);
        area->add_child(create_rect_collision(tile_size));
        area->add_child(create_themed_visual(theme, String("Switch"), tile_size, Color(0.9, 0.54, 0.16, 1.0)));
        area->set_meta("player_inside", false);
        area->set_meta("shadow_inside", false);
        area->set_meta("default_collision_layer", area->get_collision_layer());
        area->set_meta("default_collision_mask", area->get_collision_mask());
        apply_common_tile_properties(area, p_tile);
        const Dictionary props = p_tile->get_properties();
        if (props.has("message")) {
            area->set_meta("message", String(props["message"]));
        }
        area->add_to_group("mnms_switch");
        p_parent->add_child(area);
        return area;
    }

    if (block_type == "Button") {
        StaticBody2D *body = memnew(StaticBody2D);
        body->set_position(tile_position);
        body->set_collision_layer(LAYER_WORLD);
        body->set_collision_mask(0);
        body->add_child(create_rect_collision(tile_size));
        body->add_child(create_themed_visual(theme, String("Button"), tile_size, Color(0.88, 0.45, 0.18, 1.0)));
        body->set_meta("default_collision_layer", body->get_collision_layer());
        body->set_meta("default_collision_mask", body->get_collision_mask());
        body->set_meta("button_pressed", false);
        body->set_meta("active_bodies", 0);
        apply_common_tile_properties(body, p_tile);
        Area2D *detector = create_detector_area("Detector", Vector2(MAX(8.0, tile_size.x - 6.0), 16.0), Vector2(tile_size.x * 0.5, -4.0), LAYER_PLAYER_BODY | LAYER_SHADOW_BODY);
        body->add_child(detector);
        body->add_to_group("mnms_button_root");
        p_parent->add_child(body);
        return body;
    }

    if (block_type == "Exit") {
        Area2D *area = memnew(Area2D);
        area->set_position(tile_position);
        area->set_collision_layer(0);
        area->set_collision_mask(LAYER_PLAYER_BODY | LAYER_SHADOW_BODY);
        area->add_child(create_rect_collision(tile_size));
        area->add_child(create_themed_visual(theme, block_type, tile_size, Color(0.2, 0.74, 0.3, 1.0)));
        area->set_meta("default_collision_layer", area->get_collision_layer());
        area->set_meta("default_collision_mask", area->get_collision_mask());
        apply_common_tile_properties(area, p_tile);
        area->set_meta("control_mode", "exit");
        area->add_to_group("mnms_exit");
        p_parent->add_child(area);
        return area;
    }

    if (block_type == "Checkpoint") {
        Area2D *area = memnew(Area2D);
        area->set_position(tile_position);
        area->set_collision_layer(0);
        area->set_collision_mask(LAYER_PLAYER_BODY | LAYER_SHADOW_BODY);
        area->add_child(create_rect_collision(tile_size));
        area->add_child(create_themed_visual(theme, block_type, tile_size, Color(0.95, 0.86, 0.27, 1.0)));
        area->set_meta("default_collision_layer", area->get_collision_layer());
        area->set_meta("default_collision_mask", area->get_collision_mask());
        apply_common_tile_properties(area, p_tile);
        area->add_to_group("mnms_checkpoint");
        p_parent->add_child(area);
        return area;
    }

    if (block_type == "Collectable") {
        Area2D *area = memnew(Area2D);
        area->set_position(tile_position);
        area->set_collision_layer(0);
        area->set_collision_mask(LAYER_PLAYER_BODY | LAYER_SHADOW_BODY);
        area->add_child(create_rect_collision(tile_size));
        area->add_child(create_themed_visual(theme, block_type, tile_size, Color(0.98, 0.8, 0.2, 1.0)));
        area->set_meta("default_collision_layer", area->get_collision_layer());
        area->set_meta("default_collision_mask", area->get_collision_mask());
        apply_common_tile_properties(area, p_tile);
        area->add_to_group("mnms_collectable");
        p_parent->add_child(area);
        return area;
    }

    if (block_type == "NotificationBlock") {
        Area2D *area = memnew(Area2D);
        area->set_position(tile_position);
        area->set_collision_layer(0);
        area->set_collision_mask(LAYER_PLAYER_BODY | LAYER_SHADOW_BODY);
        area->add_child(create_rect_collision(tile_size));
        area->add_child(create_themed_visual(theme, block_type, tile_size, Color(0.94, 0.9, 0.55, 1.0)));
        area->set_meta("default_collision_layer", area->get_collision_layer());
        area->set_meta("default_collision_mask", area->get_collision_mask());
        apply_common_tile_properties(area, p_tile);
        const Dictionary props = p_tile->get_properties();
        if (props.has("message")) {
            area->set_meta("message", String(props["message"]));
        }
        area->add_to_group("mnms_notification");
        p_parent->add_child(area);
        return area;
    }

    if (block_type == "Pushable" || block_type == "ShadowPushable") {
        const bool shadow_only = block_type == "ShadowPushable";
        RigidBody2D *body = memnew(RigidBody2D);
        body->set_position(tile_position);
        body->set_lock_rotation_enabled(true);
        body->set_freeze_enabled(false);
        body->set_can_sleep(false);
        body->set_gravity_scale(1.0);
        body->set_linear_damp(6.0);
        body->set_angular_damp(10.0);
        body->set_collision_layer(shadow_only ? LAYER_SHADOW_WORLD : LAYER_WORLD);
        body->set_collision_mask((shadow_only ? LAYER_SHADOW_WORLD : LAYER_WORLD) | LAYER_WORLD | (shadow_only ? LAYER_SHADOW_BODY : LAYER_PLAYER_BODY));
        body->add_child(create_rect_collision(tile_size));
        body->add_child(create_themed_visual(theme, block_type, tile_size, shadow_only ? Color(0.51, 0.48, 0.82, 1.0) : Color(0.77, 0.58, 0.34, 1.0)));
        body->set_meta("default_collision_layer", body->get_collision_layer());
        body->set_meta("default_collision_mask", body->get_collision_mask());
        apply_common_tile_properties(body, p_tile);
        body->add_to_group(shadow_only ? "mnms_shadow_pushable" : "mnms_pushable");
        body->add_to_group(shadow_only ? "mnms_shadow_solid" : "mnms_solid");
        p_parent->add_child(body);
        return body;
    }

    Node2D *placeholder = memnew(Node2D);
    placeholder->set_position(tile_position);
    placeholder->add_child(create_rect_visual(tile_size, Color(0.45, 0.45, 0.45, 1.0)));
    apply_common_tile_properties(placeholder, p_tile);
    placeholder->add_to_group("mnms_unknown_block");
    p_parent->add_child(placeholder);
    return placeholder;
}
