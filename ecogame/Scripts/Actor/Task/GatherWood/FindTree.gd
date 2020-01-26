extends Task

func _init():
	self.init("FindTree")

func perform(delta : float, actor : Actor) -> bool:
	var path_to_tree = Lib.instance.navigateToClosestVoxel(actor.global_transform.origin, 4)
	self.set_task_data("path_to_tree", path_to_tree)
	return true
