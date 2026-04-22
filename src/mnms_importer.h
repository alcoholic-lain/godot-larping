// src/mnms_importer.h
#ifndef MNMS_IMPORTER_H
#define MNMS_IMPORTER_H

#include "mnms_level_resource.h"
#include "mnms_pack_resource.h"
#include "mnms_theme_resource.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>

namespace godot {

class MnmsImporter : public RefCounted {
    GDCLASS(MnmsImporter, RefCounted)

private:
    String last_error;

    bool _parse_size_value(const String &p_value, Vector2i &r_size) const;
    bool _import_levelpacks(const String &p_source_root, const String &p_output_root, Array &r_pack_paths);
    bool _import_themes(const String &p_source_root, const String &p_output_root, Array &r_theme_paths);

    Ref<MnmsPackResource> _parse_pack(const String &p_pack_id, const String &p_levels_lst_path, const String &p_output_root);
    Ref<MnmsLevelResource> _parse_map(const String &p_map_path);
    Ref<MnmsThemeResource> _parse_theme(const String &p_theme_id, const String &p_theme_file_path);

protected:
    static void _bind_methods();

public:
    MnmsImporter();
    ~MnmsImporter();

    bool import_all(const String &p_source_root, const String &p_output_root);
    String get_last_error() const;
};

} // namespace godot

#endif
