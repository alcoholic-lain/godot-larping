// src/mnms_level_resource.h
#ifndef MNMS_LEVEL_RESOURCE_H
#define MNMS_LEVEL_RESOURCE_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace godot {

class MnmsLevelResource : public Resource {
    GDCLASS(MnmsLevelResource, Resource)

private:
    String level_name;
    Vector2i world_size;
    int32_t time_limit;
    int32_t recordings_count;
    Array tiles;
    Dictionary metadata;

protected:
    static void _bind_methods();

public:
    MnmsLevelResource();
    ~MnmsLevelResource();

    void set_level_name(const String &p_level_name);
    String get_level_name() const;

    void set_world_size(const Vector2i &p_world_size);
    Vector2i get_world_size() const;

    void set_time_limit(int32_t p_time_limit);
    int32_t get_time_limit() const;

    void set_recordings_count(int32_t p_recordings_count);
    int32_t get_recordings_count() const;

    void set_tiles(const Array &p_tiles);
    Array get_tiles() const;

    void set_metadata(const Dictionary &p_metadata);
    Dictionary get_metadata() const;
};

} // namespace godot

#endif
