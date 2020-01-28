extends Task

var voxel

func _init(voxel : int):
	self.voxel = voxel
	self.init("FindPathToVoxel")

func perform(delta : float, actor) -> bool:
	var path = actor.task_handler.get_task_data("path")
	
	if path == null:
		var path_requested = actor.task_handler.get_task_data("path_requested")
		if path_requested != null && path_requested:
			return false
	
		var from = actor.global_transform.origin
#		print("actor: %s, from: %s"%[actor.get_instance_id(), from])
		Lib.instance.navigateToClosestVoxel(from, voxel, actor.get_instance_id(), "path")
		actor.task_handler.set_task_data("path_requested", true)
		return false
	
	if path.size() == 0:
		path = null
		actor.task_handler.set_task_data("path", null)
#		print("_____________PATH NOT FOUND!")
	
	actor.task_handler.set_task_data("path_requested", false)
	return true