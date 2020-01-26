extends Task

func _init():
	self.init("ChopWood")

func perform(delta : float, actor : Actor) -> bool:
	var voxel_nearby = self.get_task_data("voxel_nearby")
	if voxel_nearby:
		Lib.instance.setVoxel(voxel_nearby, 0)
	return true
