// src/mnms_tile_resource.h
#ifndef MNMS_TILE_RESOURCE_H
#define MNMS_TILE_RESOURCE_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace godot {

class MnmsTileResource : public Resource {
    GDCLASS(MnmsTileResource, Resource)

private:
    String block_type;
    Vector2i position;
    Vector2i size;
    Dictionary properties;

protected:
    static void _bind_methods();

public:
    MnmsTileResource();
    ~MnmsTileResource();

    void set_block_type(const String &p_block_type);
    String get_block_type() const;

    void set_position(const Vector2i &p_position);
    Vector2i get_position() const;

    void set_size(const Vector2i &p_size);
    Vector2i get_size() const;

    void set_properties(const Dictionary &p_properties);
    Dictionary get_properties() const;
};

} // namespace godot

#endif
