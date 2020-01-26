extends Task

var path = null
var path_key = null
var waypoint = null
var i = 0

func set_path(path : PoolVector3Array):
	self.path = path

func set_path_key(path_key : String):
	self.path_key = path_key

func _init():
	self.init("FollowPath")

func move_to_waypoint(actor):
	if waypoint == null: return
	var direction = waypoint - actor.global_transform.origin
	direction = direction.normalized()
	actor.velocity = direction * actor.MAX_SPEED

func perform(delta : float, actor : Actor) -> bool:
	if waypoint == null:
		if path_key != null:
			path = self.get_task_data(path_key)
		if path == null || path.size() == 0: return true
		waypoint = path[i]
	
	move_to_waypoint(actor)
	
	if waypoint != null && actor.global_transform.origin.distance_to(waypoint) <= 0.4:
		if path.size() > i + 1:
			i += 1
			waypoint = path[i]
		else:
			actor.velocity *= 0
			actor.acceleration *= 0
			i = 0
			waypoint = path[i]
			return true
	return false
