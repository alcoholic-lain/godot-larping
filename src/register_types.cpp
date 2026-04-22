// src/register_types.cpp
#include "register_types.h"

#include "mnms_block_system.h"
#include "mnms_game_director.h"
#include "mnms_importer.h"
#include "mnms_level_resource.h"
#include "mnms_level_runner.h"
#include "mnms_pack_resource.h"
#include "mnms_player.h"
#include "mnms_player_shadow_system.h"
#include "mnms_replay_system.h"
#include "mnms_shadow.h"
#include "mnms_theme_resource.h"
#include "mnms_tile_resource.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_mnms_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    ClassDB::register_class<MnmsTileResource>();
    ClassDB::register_class<MnmsLevelResource>();
    ClassDB::register_class<MnmsPackResource>();
    ClassDB::register_class<MnmsThemeResource>();
    ClassDB::register_class<MnmsImporter>();

    ClassDB::register_class<MnmsPlayer>();
    ClassDB::register_class<MnmsShadow>();
    ClassDB::register_class<MnmsReplaySystem>();
    ClassDB::register_class<MnmsPlayerShadowSystem>();
    ClassDB::register_class<MnmsBlockSystem>();
    ClassDB::register_class<MnmsLevelRunner>();
    ClassDB::register_class<MnmsGameDirector>();
}

void uninitialize_mnms_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

extern "C" {
GDExtensionBool GDE_EXPORT mnms_library_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    const GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization) {
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
    init_obj.register_initializer(initialize_mnms_module);
    init_obj.register_terminator(uninitialize_mnms_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
    return init_obj.init();
}
}
