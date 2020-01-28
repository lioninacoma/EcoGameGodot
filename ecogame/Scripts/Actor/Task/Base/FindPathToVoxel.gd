extends Task

var voxel

func _init(voxel : int):
	self.voxel = voxel
	self.init("FindPathToVoxel")

func perform(delta : float, actor) -> bool:
	Lib.instance.navigateToClosestVoxel(actor.global_transform.origin, voxel, actor.get_instance_id(), "path")
	var path = actor.task_handler.get_task_data("path")
	if path == null:
		return false
	if path.size() == 0:
#		print("_____________PATH NOT FOUND!")
		actor.task_handler.set_task_data("path", null)
	return true