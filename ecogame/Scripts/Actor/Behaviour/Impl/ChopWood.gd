extends Behaviour

var Tree = load("res://Scripts/Tree.gd")

func run(actor, context) -> bool:
	var position = actor.global_transform.origin
	var voxels = Lib.instance.findVoxelsInRange(position, 4, 4)
	var closest = null
	var closest_dist = INF
	if voxels.size() > 0:
		for v in voxels:
			var dist = position.distance_to(v)
			if dist < closest_dist:
				closest = v
				closest_dist = dist
	
	position = closest
	if position != null:
		Lib.instance.setVoxel(position, 0)
		
		var volume = Lib.instance.getDisconnectedVoxels(position, 6)
		if volume.size() <= 0: return true
		var meshes = Lib.instance.buildVoxelAssetByVolume(volume)
		if meshes.size() <= 0: return true
		for v in volume:
			Lib.instance.setVoxel(v.getPosition(), 0)

		var asset = Lib.game.build_asset(meshes, null)
		var tree = Tree.new(asset, volume)
		Lib.game.add_child(tree)
		var aabb = tree.get_mesh().get_aabb()
		var w = aabb.end.x - aabb.position.x
		var h = aabb.end.y - aabb.position.y
		var d = aabb.end.z - aabb.position.z

		tree.global_transform.origin.x = position.x + 0.5
		tree.global_transform.origin.y = position.y + 1 + (h/2)
		tree.global_transform.origin.z = position.z + 0.5
	return false
