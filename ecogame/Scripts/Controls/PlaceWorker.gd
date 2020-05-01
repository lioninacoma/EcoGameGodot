extends Node

onready var eco_game = get_tree().get_root().get_node("EcoGame")
onready var actor_manager = eco_game.get_node("ActorManager")
onready var controls = $"../"

#func _process(delta : float) -> void:
#	if controls.middle_pressed:
#		var actor = actor_manager.create_actor()
#		actor.global_transform.origin.x = controls.middle_pressed_location.x + 0.5
#		actor.global_transform.origin.y = controls.middle_pressed_location.y + 0.5
#		actor.global_transform.origin.z = controls.middle_pressed_location.z + 0.5
#		print("%s actors created."%[actor_manager.amount_actors()])

func _input(event : InputEvent) -> void:
	if event is InputEventMouseButton:
		if event.button_index == BUTTON_RIGHT and event.pressed and !controls.control_active:
			var from = controls.camera.project_ray_origin(event.position)
			var to = from + controls.camera.project_ray_normal(event.position) * WorldVariables.PICK_DISTANCE
			var space_state = eco_game.get_world().direct_space_state
			var result = space_state.intersect_ray(from, to)

			if result:
				var world = result.collider.shape_owner_get_owner(0)
				var actor = actor_manager.create_actor(world)
				actor.global_transform.origin = result.position
				var cog = world.get_parent().get_center_of_gravity()
				var g = (actor.transform.origin - cog).normalized()
				actor.set_rotation_to(g + result.normal)
				print("%s actors created."%[actor_manager.amount_actors()])
