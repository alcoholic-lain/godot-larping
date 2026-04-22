// src/mnms_theme_resource.h
#ifndef MNMS_THEME_RESOURCE_H
#define MNMS_THEME_RESOURCE_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {

class MnmsThemeResource : public Resource {
    GDCLASS(MnmsThemeResource, Resource)

private:
    String theme_id;
    String name;
    Dictionary block_texture_map;
    Dictionary character_texture_map;
    Dictionary character_state_map;
    Dictionary metadata;

protected:
    static void _bind_methods();

public:
    MnmsThemeResource();
    ~MnmsThemeResource();

    void set_theme_id(const String &p_theme_id);
    String get_theme_id() const;

    void set_name(const String &p_name);
    String get_name() const;

    void set_block_texture_map(const Dictionary &p_block_texture_map);
    Dictionary get_block_texture_map() const;

    void set_character_texture_map(const Dictionary &p_character_texture_map);
    Dictionary get_character_texture_map() const;

    void set_character_state_map(const Dictionary &p_character_state_map);
    Dictionary get_character_state_map() const;

    void set_metadata(const Dictionary &p_metadata);
    Dictionary get_metadata() const;
};

} // namespace godot

#endif
