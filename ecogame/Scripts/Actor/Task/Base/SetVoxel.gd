extends Task

var position
var position_key
var voxel
var voxel_key

func _init(args : Dictionary):
	self.position = args["position"] if args.has("position") else null
	self.position_key = args["position_key"] if args.has("position_key") else null
	self.voxel = args["voxel"] if args.has("voxel") else null
	self.voxel_key = args["voxel_key"] if args.has("voxel_key") else null
	self.init("SetVoxel")

func perform(delta : float, actor) -> bool:
	if finished:
		finished = false
		return true
	
	if position_key != null:
		position = actor.task_handler.get_task_data(position_key)
	
	if voxel_key != null:
		voxel = actor.task_handler.get_task_data(voxel_key)
	
	if position != null && voxel != null:
		Lib.instance.setVoxel(position, voxel)
		
		var volume = Lib.instance.getDisconnectedVoxels(position, 6)
		if volume.size() <= 0: return true
		var meshes = Lib.instance.buildVoxelAssetByVolume(volume)
		if meshes.size() <= 0: return true
		for v in volume:
			Lib.instance.setVoxel(v.getPosition(), 0)
		var asset = Lib.game.build_mesh_instance_rigid(meshes, null)
		Lib.game.add_child(asset)
		var mesh = asset.get_node("mesh")
		var w = mesh.get_aabb().end.x - mesh.get_aabb().position.x
		var d = mesh.get_aabb().end.z - mesh.get_aabb().position.z
		
		asset.global_transform.origin.x = position.x - int(w / 2)
		asset.global_transform.origin.y = position.y + 1
		asset.global_transform.origin.z = position.z - int(d / 2)
	return true
