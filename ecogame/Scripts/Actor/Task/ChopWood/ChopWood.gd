extends Task

var Tree = load("res://Scripts/Tree.gd")

var position
var position_key

func _init(args : Dictionary):
	self.position = args["position"] if args.has("position") else null
	self.position_key = args["position_key"] if args.has("position_key") else null
	self.init("ChopWood")

func perform(delta : float, actor) -> bool:
	if finished:
		finished = false
		return true
	
	if position_key != null:
		position = actor.task_handler.get_task_data(position_key)
	
	if position != null:
		Lib.instance.setVoxel(position, 0)
		
#		var volume = Lib.instance.getDisconnectedVoxels(position, 8)
#		if volume.size() <= 0: return true
#		for voxel in volume:
#			var voxelPos = voxel.getPosition()
#			var meshes = Lib.instance.buildVoxelAssetByVolume([voxel])
#			if meshes.size() <= 0: continue
#			Lib.instance.setVoxel(voxelPos, 0)
#
#			var asset = Lib.game.build_asset(meshes, null)
#			Lib.game.add_child(asset)
#			asset.global_transform.origin.x = voxelPos.x + 0.5
#			asset.global_transform.origin.y = voxelPos.y + 0.5
#			asset.global_transform.origin.z = voxelPos.z + 0.5
			
		var volume = Lib.instance.getDisconnectedVoxels(position, 6)
		if volume.size() <= 0: return true
		var meshes = Lib.instance.buildVoxelAssetByVolume(volume)
		if meshes.size() <= 0: return true
		for v in volume:
			Lib.instance.setVoxel(v.getPosition(), 0)

		var asset = Lib.game.build_asset(meshes, null)
		var tree = Tree.new(asset, volume)
		Lib.game.add_child(tree)
		var body = tree.get_body()
		var aabb = tree.get_mesh().get_aabb()
		var w = aabb.end.x - aabb.position.x
		var h = aabb.end.y - aabb.position.y
		var d = aabb.end.z - aabb.position.z

		tree.global_transform.origin.x = position.x + 0.5
		tree.global_transform.origin.y = position.y + 1 + (h/2)
		tree.global_transform.origin.z = position.z + 0.5

#		body.apply_central_impulse(Vector3(1, 1, 1) * 16)
	return true
