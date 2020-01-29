extends Task

var voxel
var voxel_key
var position
var position_key

func _init(args : Dictionary):
	self.voxel = args["voxel"] if args.has("voxel") else null
	self.voxel_key = args["voxel_key"] if args.has("voxel_key") else null
	self.position_key = args["position_key"] if args.has("position_key") else null
	self.init("FindVoxelNearby")

func perform(delta : float, actor) -> bool:
	if finished:
		actor.task_handler.set_task_data("voxel_nearby", null)
		position = null
		finished = false
		return true
	
	if voxel_key:
		voxel = actor.task_handler.get_task_data(voxel_key)
	
	if position_key:
		position = actor.task_handler.get_task_data(position_key)
	
	if position == null:
		position = actor.global_transform.origin
	
	if !voxel: return true
	
	var voxels = Lib.instance.findVoxelsInRange(position, 4, voxel)
	var closest = null
	var closest_dist = INF
	if voxels.size() > 0:
		for v in voxels:
			var dist = position.distance_to(v)
			if dist < closest_dist:
				closest = v
				closest_dist = dist
	if closest:
		actor.task_handler.set_task_data("voxel_nearby", closest)
	else:
#		print("_____________VOXEL NOT FOUND")
		actor.task_handler.set_task_data("voxel_nearby", null)
	position = null
	return true
