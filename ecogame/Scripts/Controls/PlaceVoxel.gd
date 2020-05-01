extends Node

onready var eco_game = get_tree().get_root().get_node("EcoGame")
onready var controls = $"../"

const TIME_PERIOD = 0.05 # 50ms
var time = 0
var set = false
var world

func _process(delta : float) -> void:
	time += delta
	if time > TIME_PERIOD && controls.middle_pressed:
		world.setVoxel(world.to_local(controls.middle_pressed_location), 5.0, set)
		time = 0

func _input(event : InputEvent) -> void:
	if event is InputEventKey:
		if event.scancode == KEY_KP_0:
			set = true
		elif event.scancode == KEY_KP_1:
			set = false
	elif event is InputEventMouseButton:
		if event.button_index == BUTTON_MIDDLE and event.pressed and !controls.control_active:
			var from = controls.camera.project_ray_origin(event.position)
			var to = from + controls.camera.project_ray_normal(event.position) * WorldVariables.PICK_DISTANCE
			var space_state = eco_game.get_world().direct_space_state
			var result = space_state.intersect_ray(from, to)
		
			if result:
				world = result.collider.shape_owner_get_owner(0)
