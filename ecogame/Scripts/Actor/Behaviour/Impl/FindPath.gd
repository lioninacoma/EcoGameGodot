extends Behaviour
class_name FindPath

var to_key = null

func reset(context):
	.reset(context)
	context.set("path", null)
	context.set("path_requested", false)

func run(actor, context, global_context) -> bool:
	if to_key == null:
		return false
	
	var to = context.get(to_key)
		
	if to == null:
		to = global_context.get(to_key)
		
		if to == null:
			return false
	
	var path = context.get("path")
	
	if path == null:
		var path_requested = context.get("path_requested")
		if path_requested != null && path_requested:
			return true
	
		var from = actor.global_transform.origin
		
		if from.distance_to(to) <= 0.5:
			return false
		
		Lib.instance.navigate(from, to, actor.get_instance_id())
		context.set("path_requested", true)
		return true
	
	if path.size() == 0:
		path = null
		context.set("path", null)
	
	context.set("path_requested", false)
	return false
