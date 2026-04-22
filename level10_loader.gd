extends MnmsGameDirector

func _notification(what: int) -> void:
    if what == NOTIFICATION_READY:
        call_deferred("jump_to_level", "classic", 9)
