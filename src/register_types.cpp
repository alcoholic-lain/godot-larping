#include "register_types.h"
#include "player.h"
#include "level.h"
#include "win_platform.h"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void initialize_platformer_module(ModuleInitializationLevel level) {
    if (level != MODULE_INITIALIZATION_LEVEL_SCENE) return;
    ClassDB::register_class<Player>();
    ClassDB::register_class<Level>(); 
    ClassDB::register_class<WinPlatform>();
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