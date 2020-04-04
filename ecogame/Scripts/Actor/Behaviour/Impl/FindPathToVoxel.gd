extends Behaviour
class_name FindPathToVoxel

var voxel = null

func reset(context):
	.reset(context)
	context.set("path", null)

func run(actor, context, global_context) -> bool:
	if voxel == null:
		return false
	
	var path = context.get("path")
	
	if path == null:
		var path_requested = context.get("path_requested")
		if path_requested != null && path_requested:
			return true
	
		var from = actor.global_transform.origin
		Lib.world.navigateToClosestVoxel(from, voxel, actor.get_instance_id())
		context.set("path_requested", true)
		return true
	
	if path.size() == 0:
		path = null
		context.set("path", null)
	
	context.set("path_requested", false)
	return false
