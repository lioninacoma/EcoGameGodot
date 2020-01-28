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
	if position_key != null:
		position = actor.task_handler.get_task_data(position_key)
	
	if voxel_key != null:
		voxel = actor.task_handler.get_task_data(voxel_key)
	
	if position != null && voxel != null:
		Lib.instance.setVoxel(position, voxel)
	return true
