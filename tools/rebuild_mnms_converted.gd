extends Node

const SOURCE_DATA_PATH := "res://mnms_source_data"
const OUTPUT_ROOT := "res://mnms_converted"

func _ready() -> void:
    var importer := MnmsImporter.new()
    if not importer.import_all(SOURCE_DATA_PATH, OUTPUT_ROOT):
        push_error("MNMS import error: %s" % importer.get_last_error())
        get_tree().quit(1)
        return

    print("MNMS import complete: %s -> %s" % [SOURCE_DATA_PATH, OUTPUT_ROOT])
    get_tree().quit()
