extends Node

onready var eco_game = get_tree().get_root().get_node("EcoGame")
onready var camera = $"../Head/Camera"
onready var camera_u = $"../Head/Camera_U"

var mouse_mode_captured : bool = false
var control_active : bool
var middle_pressed : bool = false
var middle_pressed_location : Vector3 = Vector3()

var building_location : Vector3 = Vector3()

func _ready() -> void:
	Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)

func get_camera():
	if not camera:
		camera = $"../Head/Camera"
	return camera

func get_camera_u():
	if not camera_u:
		camera_u = $"../Head/Camera_U"
	return camera_u

func _process(delta : float) -> void:
	if Input.is_action_just_pressed("ui_cancel"):
		if mouse_mode_captured:
			Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)
		else:
			Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)
		mouse_mode_captured = !mouse_mode_captured

func _input(event : InputEvent) -> void:
	if event is InputEventMouseMotion and middle_pressed:
		var from = camera.project_ray_origin(event.position)
		var to = from + camera.project_ray_normal(event.position) * WorldVariables.PICK_DISTANCE
		var space_state = eco_game.get_world().direct_space_state
		var result = space_state.intersect_ray(from, to)
		
		if result:
			middle_pressed_location = result.position
	elif event is InputEventKey:
		if event.scancode == KEY_CONTROL:
			control_active = event.pressed
	elif event is InputEventMouseButton:
		if event.button_index == BUTTON_MIDDLE:
			middle_pressed = event.pressed
