extends Node

onready var eco_game = get_tree().get_root().get_node("EcoGame")
onready var controls = $"../"

func _input(event : InputEvent) -> void:
	if event is InputEventMouseButton:
		if event.button_index == BUTTON_LEFT and event.pressed and !controls.control_active:
			var from = controls.camera.project_ray_origin(event.position)
			var to = from + controls.camera.project_ray_normal(event.position) * WorldVariables.PICK_DISTANCE
			var space_state = eco_game.get_world().direct_space_state
			var result = space_state.intersect_ray(from, to)
		
			if result:
				var voxel_position = controls.get_voxel_position(result)
				var world = result.collider.shape_owner_get_owner(0)
				BehaviourGlobals.global_context.set("waypoint", world.to_local(voxel_position))
