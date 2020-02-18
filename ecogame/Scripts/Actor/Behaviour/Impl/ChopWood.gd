extends Behaviour

var Tree = load("res://Scripts/Tree.gd")

func reset(context):
	.reset(context)

func run(actor, context, global_context) -> bool:
	var tree_location = context.get("next_voxel_location")
	
	if tree_location != null:
		Lib.instance.setVoxel(tree_location, 0)
		context.set("next_voxel_location", null)
		
		var volume = Lib.instance.getDisconnectedVoxels(tree_location, 6)
		if volume.size() <= 0: return false
		for v in volume:
			if v.getType() != 5:
				return false
		var meshes = Lib.instance.buildVoxelAssetByVolume(volume)
		if meshes.size() <= 0: return false
		for v in volume:
			Lib.instance.setVoxel(v.getPosition(), 0)

		var asset = Lib.game.build_asset(meshes, null)
		var tree = Tree.new(asset, volume)
		Lib.game.add_child(tree)
		var aabb = tree.get_mesh().get_aabb()
		var w = aabb.end.x - aabb.position.x
		var h = aabb.end.y - aabb.position.y
		var d = aabb.end.z - aabb.position.z
		
		var body = tree.get_body()
		body.set_axis_lock(PhysicsServer.BODY_AXIS_ANGULAR_X, true)
		body.set_axis_lock(PhysicsServer.BODY_AXIS_ANGULAR_Y, true)
		body.set_axis_lock(PhysicsServer.BODY_AXIS_ANGULAR_Z, true)
		body.set_collision_layer(0)
		body.set_collision_mask(0)

		tree.global_transform.origin.x = tree_location.x + 0.5
		tree.global_transform.origin.y = tree_location.y + 1 + (h/2)
		tree.global_transform.origin.z = tree_location.z + 0.5
	return false
