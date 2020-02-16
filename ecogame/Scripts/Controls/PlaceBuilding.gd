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

		var fits = Lib.instance.voxelAssetFits(voxel_position, asset_type)
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
					var fits = Lib.instance.voxelAssetFits(voxel_position, asset_type)
					if !fits: return
					Lib.instance.addVoxelAsset(voxel_position, asset_type)
					controls.building_location = voxel_position
					current_asset.queue_free()
					current_asset = null
					asset_type = -1
				else:
					asset_type = 1
					var meshes = Lib.instance.buildVoxelAssetByType(asset_type)
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
