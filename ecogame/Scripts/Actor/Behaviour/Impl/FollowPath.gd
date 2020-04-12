extends Behaviour

var path = null
var waypoint = null

func reset(context):
	.reset(context)
	path = null
	waypoint = null
	context.set("path", null)

func move_to_waypoint(actor):
	if waypoint == null: return
	var world = actor.get_world()
	var direction = world.to_global(waypoint) - actor.global_transform.origin
	direction = direction.normalized()
	actor.velocity = direction * actor.MAX_SPEED

func run(actor, context, global_context) -> bool:
	if path == null:
		path = context.get("path")
		
		if path != null:
			if path.size() == 0:
				actor.velocity *= 0
				actor.acceleration *= 0
				context.set("path", null)
				return false
			else:
				waypoint = path[0]
				path.remove(0)
	
	move_to_waypoint(actor)
	var world = actor.get_world()
	
	if waypoint != null && actor.global_transform.origin.distance_to(world.to_global(waypoint)) <= 0.5:
		if path.size() > 0:
			waypoint = path[0]
			path.remove(0)
		else:
			actor.velocity *= 0
			actor.acceleration *= 0
			context.set("path", null)
			return false
	return true
