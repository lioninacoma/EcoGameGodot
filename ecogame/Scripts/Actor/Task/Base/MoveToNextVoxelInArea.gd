extends Task

var voxel

func _init(voxel):
	self.voxel = voxel
	self.init("MoveToNextVoxelInArea")

func perform(delta : float, actor) -> bool:
	if finished:
		actor.task_handler.set_task_data("next_voxel", null)
		finished = false
		return true

	var voxels = actor.task_handler.get_task_data("voxels_in_area")
	var next_voxel = null
	
	if voxels != null && voxels.size() > 0:
		var position = actor.global_transform.origin
		
		while next_voxel == null && voxels.size() > 0:
			var i = 0
			var closest = null
			var closest_i = -1
			var closest_dist = INF
			
			for v in voxels:
				var dist = position.distance_to(v)
				if dist < closest_dist:
					closest = v
					closest_dist = dist
					closest_i = i
				i += 1
			
			voxels.remove(closest_i)
			var nearby = Lib.instance.findVoxelsInRange(closest, 4, voxel)
			if nearby.size() > 0:
				next_voxel = closest
		
		if voxels.size() > 0:
			actor.task_handler.set_task_data("voxels_in_area", voxels)
		else:
			actor.task_handler.set_task_data("voxels_in_area", null)
	
	if next_voxel:
		actor.task_handler.set_task_data("next_voxel", next_voxel)
	else:
		actor.task_handler.set_task_data("next_voxel", null)
	
	return true