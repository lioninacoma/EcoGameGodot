extends Behaviour
class_name FindVoxelNearby

var voxel = null

func reset(context):
	.reset(context)
	context.set("voxel_nearby", null)

func run(actor, context, global_context) -> bool:
	if voxel == null:
		return false
	
	var position = actor.global_transform.origin
	var voxels = Lib.instance.findVoxelsInRange(position, 4, voxel)
	var closest = null
	var closest_dist = INF
	if voxels.size() > 0:
		for v in voxels:
			var dist = position.distance_to(v)
			if dist < closest_dist:
				closest = v
				closest_dist = dist
	
	context.set("voxel_nearby", closest)
	return false
