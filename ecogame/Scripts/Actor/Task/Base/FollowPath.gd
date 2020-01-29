extends Task

var path
var path_key
var waypoint = null

func _init(args : Dictionary):
	self.path = args["path"] if args.has("path") else null
	self.path_key = args["path_key"] if args.has("path_key") else null
	self.init("FollowPath")

func move_to_waypoint(actor):
	if waypoint == null: return
	var direction = waypoint - actor.global_transform.origin
	direction = direction.normalized()
	actor.velocity = direction * actor.MAX_SPEED

func perform(delta : float, actor) -> bool:
	if finished:
		actor.velocity *= 0
		actor.acceleration *= 0
		waypoint = null
		actor.task_handler.set_task_data("path", null)
		finished = false
		return true
	
	if waypoint == null:
		if path_key != null:
			path = actor.task_handler.get_task_data(path_key)
		if path == null || path.size() == 0:
			path = null
			actor.task_handler.set_task_data("path", null)
			return true
		waypoint = path[0]
		path.remove(0)
	
	move_to_waypoint(actor)
	
	if waypoint != null && actor.global_transform.origin.distance_to(waypoint) <= 0.4:
		if path.size() > 0:
			waypoint = path[0]
			path.remove(0)
		else:
			actor.velocity *= 0
			actor.acceleration *= 0
			waypoint = null
			actor.task_handler.set_task_data("path", null)
			return true
	return false
