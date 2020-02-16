extends Behaviour
class_name FindNextVoxel

var voxel = null

func reset(context):
	.reset(context)
	context.set("next_voxel_location", null)

func run(actor, context) -> bool:
	if voxel == null:
		return false
	
	var voxels = context.get("voxels_in_area")
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
		
		if voxels.size() == 0:
			context.set("voxels_in_area", null)
	
	if next_voxel:
		context.set("next_voxel_location", next_voxel)
	else:
		context.set("next_voxel_location", null)
	
	return false
