extends Task

var voxel
var voxel_key = null

func set_voxel(voxel : int):
	self.voxel = voxel

func set_voxel_key(voxel_key : String):
	self.voxel_key = voxel_key

func _init():
	self.init("FindVoxelNearby")

func perform(delta : float, actor : Actor) -> bool:
	if voxel_key:
		voxel = self.get_task_data(voxel_key)
	
	if !voxel: return true
	
	var start : Vector3 = actor.global_transform.origin
	var voxels = Lib.instance.findVoxelsInRange(start, 2, voxel)
	var closest = null
	var closest_dist = INF
	if voxels.size() > 0:
		for v in voxels:
			var dist = start.distance_to(v)
			if dist < closest_dist:
				closest = v
				closest_dist = dist
	if closest:
		set_task_data("voxel_nearby", closest)
	return true
