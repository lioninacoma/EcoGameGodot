extends Task

var voxel
var position
var position_key
var size

func _init(args : Dictionary):
	self.voxel = args["voxel"] if args.has("voxel") else null
	self.position_key = args["position_key"] if args.has("position_key") else null
	self.size = args["size"] if args.has("size") else 16
	self.init("FindVoxelsInArea")

func perform(delta : float, actor) -> bool:
	if finished:
		actor.task_handler.set_task_data("voxels_in_area", null)
		position = null
		finished = false
		return true
	
	var voxels = actor.task_handler.get_task_data("voxels_in_area")
	if voxels != null && voxels.size() > 0:
		return true
	
	if position_key:
		position = actor.task_handler.get_task_data(position_key)
	
	if position == null:
		position = actor.global_transform.origin
	
	if !voxel: return true
	
	var start = position + Vector3(-size, 0, -size)
	var end = position + Vector3(size, 0, size)
	voxels = Lib.instance.getVoxelsInArea(start, end, voxel)
	
	if voxels.size() > 0:
		actor.task_handler.set_task_data("voxels_in_area", voxels)
	else:
		actor.task_handler.set_task_data("voxels_in_area", null)
	position = null
	return true
