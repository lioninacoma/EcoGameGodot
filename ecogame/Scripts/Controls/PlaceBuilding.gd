extends Node

onready var eco_game = get_tree().get_root().get_node("EcoGame")
onready var controls = $"../"

var current_asset = null
var asset_type : int = -1

func preview_current_asset(at : Vector2):
	var from = controls.camera.project_ray_origin(at)
	var to = from + controls.camera.project_ray_normal(at) * WorldVariables.PICK_DISTANCE
	var space_state = eco_game.get_world().direct_space_state
	var result = space_state.intersect_ray(from, to)
	
	if result:
		var voxel_position = controls.get_voxel_position(result)
		var aabb = current_asset.get_aabb()
		var h = aabb.end.y - aabb.position.y

		current_asset.global_transform.origin.x = voxel_position.x
		current_asset.global_transform.origin.y = voxel_position.y + int(h/2) + 1
		current_asset.global_transform.origin.z = voxel_position.z

		var fits = Lib.world.voxelAssetFits(voxel_position, asset_type)
		for i in range(current_asset.get_surface_material_count()):
			var material = current_asset.mesh.surface_get_material(i)
			var color : Color
			if fits: color = Color(0, 1, 0)
			else:    color = Color(1, 0, 0)
			material.set_blend_mode(1)
			material.albedo_color = color

func _input(event : InputEvent) -> void:
	if event is InputEventMouseMotion and current_asset:
		preview_current_asset(event.position)
	elif event is InputEventMouseButton:
		if event.button_index == BUTTON_LEFT and event.pressed and !controls.control_active:
			var from = controls.camera.project_ray_origin(event.position)
			var to = from + controls.camera.project_ray_normal(event.position) * WorldVariables.PICK_DISTANCE
			var space_state = eco_game.get_world().direct_space_state
			var result = space_state.intersect_ray(from, to)
		
			if result:
				var voxel_position = controls.get_voxel_position(result)
				
				if current_asset:
					var fits = Lib.world.voxelAssetFits(voxel_position, asset_type)
					if !fits: return
					Lib.world.addVoxelAsset(voxel_position, asset_type)
					
					current_asset.queue_free()
					current_asset = null
					asset_type = -1
					
					var size = 64
					var start = voxel_position + Vector3(-size, 0, -size)
					var end = voxel_position + Vector3(size, 0, size)
					var voxels_in_area = Lib.world.getVoxelsInArea(start, end, 4)
					
					BehaviourGlobals.global_context.set("voxels_in_area", voxels_in_area)
					BehaviourGlobals.global_context.set("storehouse_location", voxel_position)
				else:
					asset_type = 1
					var meshes = Lib.world.buildVoxelAssetByType(asset_type)
					current_asset = eco_game.build_mesh_instance(meshes, null)
					var body = current_asset.get_node("body")
					current_asset.remove_child(body)
					
					for i in range(current_asset.get_surface_material_count()):
						var material = current_asset.mesh.surface_get_material(i).duplicate(true)
						material.set_blend_mode(1)
						material.albedo_color = Color(0, 1, 0)
						current_asset.mesh.surface_set_material(i, material)
					
					eco_game.add_child(current_asset)
					var aabb = current_asset.get_aabb()
					var h = aabb.end.y - aabb.position.y
					current_asset.global_transform.origin.x = voxel_position.x
					current_asset.global_transform.origin.y = voxel_position.y + int(h/2) + 1
					current_asset.global_transform.origin.z = voxel_position.z
