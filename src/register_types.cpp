#include "register_types.h"
#include "player.h"
#include "level.h"
#include "win_platform.h"
#include "win_screen.h"
#include "main_menu.h"
#include "frame.h"
#include "platform.h"
#include "collectable.h"
#include "orb.h"
#include "spike.h"
#include "lose_screen.h"
#include "kill_platform.h"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void initialize_platformer_module(ModuleInitializationLevel level) {
    if (level != MODULE_INITIALIZATION_LEVEL_SCENE) return;
    ClassDB::register_class<Frame>();    // must come before Level (which inherits Frame)
    ClassDB::register_class<Player>();
    ClassDB::register_class<Level>();
    ClassDB::register_class<Platform>();
    ClassDB::register_class<WinPlatform>();
    ClassDB::register_class<WinScreen>();
    ClassDB::register_class<MainMenu>();
    ClassDB::register_class<Collectable>();
    ClassDB::register_class<Orb>();
    ClassDB::register_class<Spike>();
    ClassDB::register_class<LoseScreen>();
    ClassDB::register_class<KillPlatform>();
}

void uninitialize_platformer_module(ModuleInitializationLevel level) {}

extern "C" {
GDExtensionBool GDE_EXPORT platformer_library_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    const GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization)
{
    godot::GDExtensionBinding::InitObject init_obj(
        p_get_proc_address, p_library, r_initialization);
    init_obj.register_initializer(initialize_platformer_module);
    init_obj.register_terminator(uninitialize_platformer_module);
    init_obj.set_minimum_library_initialization_level(
        MODULE_INITIALIZATION_LEVEL_SCENE);
    return init_obj.init();
}
}