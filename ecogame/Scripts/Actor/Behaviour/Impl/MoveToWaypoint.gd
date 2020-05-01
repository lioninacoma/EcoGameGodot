extends Behaviour
class_name MoveToWaypoint

onready var eco_game = get_tree().get_root().get_node("EcoGame")

var path = null
var waypoint = null

func reset(context):
	.reset(context)
	path = null
	waypoint = null
	context.set("path_requested", false)
	context.set("path", null)

func move_to_waypoint(actor):
	if waypoint == null: return
	
	var world = actor.get_voxel_world()
	var waypoint_global = world.to_global(waypoint)
	var direction = waypoint_global - actor.global_transform.origin
	
	direction = direction.normalized()
	actor.velocity = direction * actor.MAX_SPEED

func request_path(actor, context, global_context):
	var to = context.get("waypoint")
	if to == null:
		to = global_context.get("waypoint")
	if to == null: return
	
	var path_requested = context.get("path_requested")	
	if path_requested != null && path_requested: return
	
	var world = actor.get_voxel_world()
	var from = actor.global_transform.origin
	from = world.to_local(from)
	if from.distance_to(to) <= 0.5: return
	
	world.navigate(from, to, actor.get_instance_id())
	context.set("path_requested", true)

func run(actor, context, global_context) -> bool:	
	if waypoint == null && path == null:
		path = context.get("path")
		if path == null:
			request_path(actor, context, global_context)
			return true
		else:
			context.set("path_requested", false)
			context.set("path", null)
			
			if path.size() > 0:
				waypoint = path[0]
				path.remove(0)
			else:
				waypoint = null
				path = null
				return true
	
	var world = actor.get_voxel_world()
	move_to_waypoint(actor)
	
	if waypoint != null && actor.global_transform.origin.distance_to(world.to_global(waypoint)) <= 0.5:
		if path != null && path.size() > 0:
			waypoint = path[0]
			path.remove(0)
		else:
			actor.velocity *= 0
			actor.acceleration *= 0
			waypoint = null
			path = null
	
	return true
