#include "platform.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/polygon2d.hpp>

using namespace godot;

void Platform::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_plat_size"),        &Platform::get_plat_size);
    ClassDB::bind_method(D_METHOD("get_plat_color"),       &Platform::get_plat_color);
    ClassDB::bind_method(D_METHOD("set_plat_size",  "s"),  &Platform::set_plat_size);
    ClassDB::bind_method(D_METHOD("set_plat_color", "c"),  &Platform::set_plat_color);

    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "plat_size"),  "set_plat_size",  "get_plat_size");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR,   "plat_color"), "set_plat_color", "get_plat_color");
}

void Platform::_ready() {
    rebuild();
}

void Platform::rebuild() {
    for (int i = get_child_count() - 1; i >= 0; i--)
        get_child(i)->queue_free();

    float w = plat_size.x;
    float h = plat_size.y;

    // Collision (unchanged — keeps exact physics shape)
    CollisionShape2D *col = memnew(CollisionShape2D);
    Ref<RectangleShape2D> rect = memnew(RectangleShape2D);
    rect->set_size(plat_size);
    col->set_shape(rect);
    add_child(col);

    // Visual — rounded rectangle via Polygon2D, centred on origin
    // Corner radius: half the height so short platforms become pill-shaped,
    // capped at 12px so wide platforms don't over-round.
    float R    = Math::min(h * 0.5f, 12.0f);
    float hw   = w * 0.5f;
    float hh   = h * 0.5f;
    int   SEGS = 8;

    PackedVector2Array points;

    struct Corner { float cx, cy, a0; };
    Corner corners[4] = {
        {  hw - R,  hh - R,  0.0f           },
        { -hw + R,  hh - R,  Math_PI * 0.5f },
        { -hw + R, -hh + R,  Math_PI        },
        {  hw - R, -hh + R,  Math_PI * 1.5f },
    };

    for (auto &c : corners) {
        for (int i = 0; i <= SEGS; i++) {
            float angle = c.a0 + (Math_PI * 0.5f) * (float)i / (float)SEGS;
            points.push_back(Vector2(
                c.cx + Math::cos(angle) * R,
                c.cy + Math::sin(angle) * R
            ));
        }
    }

    Polygon2D *vis = memnew(Polygon2D);
    vis->set_polygon(points);
    vis->set_color(plat_color);
    add_child(vis);
}
