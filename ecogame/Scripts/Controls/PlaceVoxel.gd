extends Node

var Tree = load("res://Scripts/Tree.gd")

onready var eco_game = get_tree().get_root().get_node("EcoGame")
onready var controls = $"../"

var voxel = 0

func _input(event : InputEvent) -> void:
	if event is InputEventKey:
		if event.scancode == KEY_KP_0:
			voxel = 0
		elif event.scancode == KEY_KP_1:
			voxel = 1
		elif event.scancode == KEY_KP_2:
			voxel = 2
		elif event.scancode == KEY_KP_3:
			voxel = 3
		elif event.scancode == KEY_KP_4:
			voxel = 4
		elif event.scancode == KEY_KP_5:
			voxel = 5
	elif event is InputEventMouseButton:
		if event.button_index == BUTTON_LEFT and event.pressed and controls.control_active:
			var from = controls.camera.project_ray_origin(event.position)
			var to = from + controls.camera.project_ray_normal(event.position) * WorldVariables.PICK_DISTANCE
			var space_state = eco_game.get_world().direct_space_state
			var result = space_state.intersect_ray(from, to)
		
			if result:
				var voxel_position = controls.get_voxel_position(result)
				if voxel > 0:
					voxel_position += result.normal
				Lib.instance.setVoxel(voxel_position, voxel)
				
#				var volume = Lib.instance.getDisconnectedVoxels(voxel_position, 8)
#				if volume.size() <= 0: return
#				var meshes = Lib.instance.buildVoxelAssetByVolume(volume)
#				if meshes.size() <= 0: return
#				for v in volume:
#					Lib.instance.setVoxel(v.getPosition(), 0)
#
#				var asset = eco_game.build_asset(meshes, null)
#				var tree = Tree.new(asset, volume)
#				eco_game.add_child(tree)
#				var aabb = tree.get_mesh().get_aabb()
#				var w = aabb.end.x - aabb.position.x
#				var h = aabb.end.y - aabb.position.y
#				var d = aabb.end.z - aabb.position.z
#
#				tree.global_transform.origin.x = voxel_position.x + 0.5
#				tree.global_transform.origin.y = voxel_position.y + 1 + (h/2)
#				tree.global_transform.origin.z = voxel_position.z + 0.5
