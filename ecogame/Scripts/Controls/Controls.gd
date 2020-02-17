extends Node

onready var eco_game = get_tree().get_root().get_node("EcoGame")
onready var camera = $"../Head/Camera"

var mouse_mode_captured : bool = false
var control_active : bool
var middle_pressed : bool = false
var middle_pressed_location : Vector3 = Vector3()

var building_location : Vector3 = Vector3()

func _ready() -> void:
	Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)

func _process(delta : float) -> void:
	if Input.is_action_just_pressed("ui_cancel"):
		if mouse_mode_captured:
			Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)
		else:
			Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)
		mouse_mode_captured = !mouse_mode_captured

func get_voxel_position(result):
	var chunk = result.collider.shape_owner_get_owner(0)
	if not chunk: return
	
	var voxel_position = result.position
	var normal = result.normal
	var b = 0.1
	var vx = int(voxel_position.x - (b if normal.x > 0 else 0))
	var vy = int(voxel_position.y - (b if normal.y > 0 else 0))
	var vz = int(voxel_position.z - (b if normal.z > 0 else 0))
	return Vector3(vx, vy, vz)

func _input(event : InputEvent) -> void:
	if event is InputEventMouseMotion and middle_pressed:
		var from = camera.project_ray_origin(event.position)
		var to = from + camera.project_ray_normal(event.position) * WorldVariables.PICK_DISTANCE
		var space_state = eco_game.get_world().direct_space_state
		var result = space_state.intersect_ray(from, to)
		
		if result:
			var voxel_position = get_voxel_position(result)
			middle_pressed_location = voxel_position
	elif event is InputEventKey:
		if event.scancode == KEY_CONTROL:
			control_active = event.pressed
	elif event is InputEventMouseButton:
		if event.button_index == BUTTON_MIDDLE:
			middle_pressed = event.pressed
