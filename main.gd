extends Node2D

@onready var circle = $Circle
@onready var exit_button = $ExitButton

const SPEED = 300

func _ready():
	circle.position = Vector2(400, 300)
	
	exit_button.position = Vector2(20, 20)
	exit_button.size = Vector2(100, 40)
	exit_button.text = "Quitter"
	
	var points = PackedVector2Array()
	var radius = 40.0
	for i in range(64):
		var angle = i * TAU / 64
		points.append(Vector2(cos(angle), sin(angle)) * radius)
	circle.polygon = points
	
	exit_button.pressed.connect(_on_exit_pressed)

func _process(delta):
	var velocity = Vector2.ZERO
	
	if Input.is_action_pressed("ui_right"):
		velocity.x += 1
	if Input.is_action_pressed("ui_left"):
		velocity.x -= 1
	if Input.is_action_pressed("ui_down"):
		velocity.y += 1
	if Input.is_action_pressed("ui_up"):
		velocity.y -= 1
	
	if velocity.length() > 0:
		velocity = velocity.normalized()
	
	circle.position += velocity * SPEED * delta

func _on_exit_pressed():
	get_tree().quit()
