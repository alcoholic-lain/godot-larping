// src/mnms_pack_resource.h
#ifndef MNMS_PACK_RESOURCE_H
#define MNMS_PACK_RESOURCE_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {

class MnmsPackResource : public Resource {
    GDCLASS(MnmsPackResource, Resource)

private:
    String pack_id;
    String name;
    String description;
    String theme_id;
    String music_list;
    String congratulations;
    Array levels;
    Dictionary metadata;

protected:
    static void _bind_methods();

public:
    MnmsPackResource();
    ~MnmsPackResource();

    void set_pack_id(const String &p_pack_id);
    String get_pack_id() const;

    void set_name(const String &p_name);
    String get_name() const;

    void set_description(const String &p_description);
    String get_description() const;

    void set_theme_id(const String &p_theme_id);
    String get_theme_id() const;

    void set_music_list(const String &p_music_list);
    String get_music_list() const;

    void set_congratulations(const String &p_congratulations);
    String get_congratulations() const;

    void set_levels(const Array &p_levels);
    Array get_levels() const;

    void set_metadata(const Dictionary &p_metadata);
    Dictionary get_metadata() const;
};

} // namespace godot

#endif
