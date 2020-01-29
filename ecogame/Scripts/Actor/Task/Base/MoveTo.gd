extends Task

var path = null
var waypoint = null
var to : Vector3 = Vector3()

func _init(to : Vector3):
	self.to = to
	self.init("MoveTo")

func move_to_waypoint(actor):
	if waypoint == null: return
	var direction = waypoint - actor.global_transform.origin
	direction = direction.normalized()
	actor.velocity = direction * actor.MAX_SPEED

func perform(delta : float, actor) -> bool:
	if finished:
		actor.velocity *= 0
		actor.acceleration *= 0
		path = null
		waypoint = null
		actor.task_handler.set_task_data("path", null)
		finished = false
		return true
	
	if path == null:
		path = actor.task_handler.get_task_data("path")
		
		if path == null:
			var path_requested = actor.task_handler.get_task_data("path_requested")
			if path_requested != null && path_requested:
				return false
			
			var from = actor.global_transform.origin
#			print("actor: %s, from: %s, to: %s"%[actor.get_instance_id(), from, to])
			Lib.instance.navigate(from, to, actor.get_instance_id(), "path")
			actor.task_handler.set_task_data("path_requested", true)
			return false
		else:
			actor.task_handler.set_task_data("path_requested", false)
		
		if path.size() == 0:
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
			path = null
			waypoint = null
			actor.task_handler.set_task_data("path", null)
			return true
	return false
