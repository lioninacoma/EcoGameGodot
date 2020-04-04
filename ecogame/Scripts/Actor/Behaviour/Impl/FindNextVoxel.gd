extends Behaviour
class_name FindNextVoxel

var voxel = null

func reset(context):
	.reset(context)

func run(actor, context, global_context) -> bool:
	if voxel == null:
		return false
	
	var next_voxel = null
	var voxels = global_context.get("voxels_in_area")
	
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
			global_context.set("voxels_in_area", voxels)
			
			var nearby = Lib.world.findVoxelsInRange(closest, 4, voxel)
			if nearby.size() > 0:
				next_voxel = closest
	
	context.set("next_voxel_location", next_voxel)
	return false
