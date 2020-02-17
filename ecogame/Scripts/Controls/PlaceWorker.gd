extends Node

onready var eco_game = get_tree().get_root().get_node("EcoGame")
onready var actor_manager = eco_game.get_node("ActorManager")
onready var controls = $"../"

func _process(delta : float) -> void:
	if controls.middle_pressed:
		var actor = actor_manager.create_actor()
		actor.global_transform.origin.x = controls.middle_pressed_location.x + 0.5
		actor.global_transform.origin.y = controls.middle_pressed_location.y + 1
		actor.global_transform.origin.z = controls.middle_pressed_location.z + 0.5
		print("%s actors created."%[actor_manager.amount_actors()])

func _input(event : InputEvent) -> void:
	if event is InputEventMouseButton:
		if event.button_index == BUTTON_RIGHT and event.pressed:
			var from = controls.camera.project_ray_origin(event.position)
			var to = from + controls.camera.project_ray_normal(event.position) * WorldVariables.PICK_DISTANCE
			var space_state = eco_game.get_world().direct_space_state
			var result = space_state.intersect_ray(from, to)
		
			if result:
				var voxel_position = controls.get_voxel_position(result)
				var actor = actor_manager.create_actor()
				actor.global_transform.origin.x = voxel_position.x + 0.5
				actor.global_transform.origin.y = voxel_position.y + 1
				actor.global_transform.origin.z = voxel_position.z + 0.5
				print("%s actors created."%[actor_manager.amount_actors()])
