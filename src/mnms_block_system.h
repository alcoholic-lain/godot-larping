// src/mnms_block_system.h
#ifndef MNMS_BLOCK_SYSTEM_H
#define MNMS_BLOCK_SYSTEM_H

#include "mnms_theme_resource.h"
#include "mnms_tile_resource.h"

#include <godot_cpp/classes/node.hpp>

namespace godot {

class MnmsBlockSystem : public Node {
    GDCLASS(MnmsBlockSystem, Node)

private:
    Ref<MnmsThemeResource> theme;

protected:
    static void _bind_methods();

public:
    MnmsBlockSystem();
    ~MnmsBlockSystem();

    void set_theme(const Ref<MnmsThemeResource> &p_theme);
    Ref<MnmsThemeResource> get_theme() const;
    Node *spawn_tile(const Ref<MnmsTileResource> &p_tile, Node *p_parent);
};

} // namespace godot

#endif
